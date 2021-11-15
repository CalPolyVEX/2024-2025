//This file contains the navigation node

#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <image_transport/image_transport.h>
#include <getopt.h>
#include "navigation.h"

using namespace std;

#define NUM_GROUND 80
#define NUM_LOC 64
#define NUM_TURN 1
#define NUM_GOAL 2

#define LEFT_OBSTACLE_X 256
#define LEFT_OBSTACLE_Y 229
#define RIGHT_OBSTACLE_X 384
#define RIGHT_OBSTACLE_Y 229

int sim_mode = 0;
int debug_mode = 1;
ros::NodeHandle* h;
ros::Timer goal_timer;

Navigation::Navigation() : it(nh) {
   //subscriber for pose
   if (sim_mode == 1) {
      odom_data_sub = nh.subscribe("/camera/odom/sample", 1, &Navigation::odom_callback, this);
   } else {
      odom_data_sub = nh.subscribe("/ekf_node/odom", 1, &Navigation::odom_callback, this);
   }

   //subscriber for autonomous mode signal
   autonomous_sub = nh.subscribe("/autonomous", 1, &Navigation::autonomous_mode_callback, this);

   //subscriber for navigation goal
   nav_goal_sub = nh.subscribe("/nav_goal", 1, &Navigation::send_goal, this);

   //subscriber for neural network data
   nn_data_sub = nh.subscribe("/nn_data", 1, &Navigation::nn_data_callback, this);

   //subscriber for 640x360 image
   image_transport::TransportHints hints("compressed");

   if (sim_mode == 0) {
#ifdef __x86_64__
      img_sub = it.subscribe("/see3cam_cu20/image_raw", 1, &Navigation::img_callback, this, hints);
#else
      //if on ARM, read live from the camera
      img_sub = it.subscribe("/see3cam_cu20/image_raw_live", 1, &Navigation::img_callback, this);
#endif
   } else {
      img_sub = it.subscribe("/see3cam_cu20/image_raw", 1, &Navigation::img_callback, this, hints);
   }

   image_pub_ = it.advertise("/nav_output_video", 1);
   twist_pub_ = nh.advertise<geometry_msgs::Twist>("/nav_cmd_vel", 1);

   // point cloud publisher
   pointcloud_pub_ = nh.advertise<sensor_msgs::PointCloud2>("/point_cloud", 1);

   // control board publisher
   control_board_pub_ = nh.advertise<std_msgs::Int32MultiArray>("/control_board", 1);

   nh.setParam("/autonomous_mode", false);

   new_image = cv::Mat(360, 640, CV_8UC3);

   //tell the action client that we want to spin a thread by default
   tfListener = new tf2_ros::TransformListener(tfBuffer);
   h = &nh; //set the ROS node handle

   //initialize the hardcoded turn information
   init_turn_transforms();

   //create the move base client (this client manages how to drive to the goal)
   action_client = new MoveBaseClient("move_base", true);
   std::this_thread::sleep_for(std::chrono::milliseconds(100)); //wait for a little bit

   //wait for the action server to come up
   while(!action_client->waitForServer(ros::Duration(5.0))){
      ROS_INFO("Waiting for the move_base action server to come up");
   }

   //set the localization position arrays to 0
   for(int i=0; i<LOCALIZATION_ARRAY_SIZE; i++) {
      localization_num[i] = 0;
      localization_value[i] = 0;

      localization_pose[i].position.x = 0;
      localization_pose[i].position.y = 0;
      localization_pose[i].position.z = 0;

      localization_pose[i].orientation.w = 1;
      localization_pose[i].orientation.x = 0;
      localization_pose[i].orientation.y = 0;
      localization_pose[i].orientation.z = 0;

      localization_heading[i] = 0;
      localization_turn_progress[i] = 0;
   }

   //start the turn transform thread
   turn_transform_thread = new std::thread(&Navigation::update_turn_transform, this);

   //initialize the route to all invalid
   for(int i=0; i<100; i++) {
      route_hallway[i] = -1;
      route_turn[i] = -1;
   }
}

//each time an image is published from the camera reader, this callback is run
void Navigation::img_callback(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImageConstPtr cv_ptr;

    try {
      cv_ptr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::BGR8);
    } catch (cv_bridge::Exception& e) {
      ROS_ERROR("cv_bridge exception: %s", e.what());
    }
    
   //resize image to IMG_HEIGHT x IMG_WIDTH if necessary
   if (cv_ptr->image.rows == 360 && cv_ptr->image.cols == 640) {
      img_mutex.lock();
      memcpy(new_image.data, cv_ptr->image.data, 640*360*3);
      img_mutex.unlock();
   } else {
      img_mutex.lock();
      cv::resize(cv_ptr->image, new_image, cv::Size(640,360), CV_INTER_LINEAR);
      img_mutex.unlock();
   }
}

//each time the neural network data is published, this callback is run
void Navigation::nn_data_callback(const std_msgs::Float64MultiArray::ConstPtr& nn_msg) {
   double turn_confidence;
   float cur_loc_conf;

   //store pointers to the neural network output
   ground = (double*) &(nn_msg->data[0]);
   loc = (double*) &(nn_msg->data[NUM_GROUND]);
   turn = (double*) &(nn_msg->data[NUM_GROUND + NUM_LOC]);
   goal = (double*) &(nn_msg->data[NUM_GROUND + NUM_TURN + NUM_LOC]);
   inference_time = (double) nn_msg->data[NUM_GROUND + NUM_TURN + NUM_LOC + 1];

   //update the localization estimate
   cur_loc_mutex.lock();
   compute_localization(&cur_loc_estimate, &cur_loc_conf);
   cur_loc_mutex.unlock();

   update_goal_transform(); //update the transform from current location to the goal
  
   //compute the localization probability
   compute_loc_prob();

   //connect the boundary points with lines
   if (debug_mode) {
      //draw the ground boundary points
      img_mutex.lock();
      for (int i=0; i<NUM_GROUND; i++) {
         cv::circle( new_image,
               cv::Point(i*8, int(ground[i] * 360)),
               2,
               cv::Scalar( 0, 0, 255 ),
               cv::FILLED,
               cv::LINE_8 );
      }
      img_mutex.unlock();

      connect_boundary();
      /* draw_lines(); */
      draw_goal();
      write_text();
   }

   //publish pointcloud
   publish_pointcloud();

   //publish the image (if in debug mode)
   if(debug_mode) {
      img_mutex.lock();
      sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
      img_mutex.unlock();
      image_pub_.publish(pub_msg);
   }

   turn_counter++;
   char coord[2];
   char lcd_msg[32];

   if (turn_counter > 2) {
      turn_counter = 0;

      coord[0] = 0; //col 0
      coord[1] = 0; //row 0
      create_control_board_msg(1,coord);
      usleep(10);

      //print localization
      if (compute_turn_prob(&turn_confidence) == 1) {
         sprintf(lcd_msg,"%02d %.2f T %.2f  ", cur_loc_estimate, cur_loc_conf, turn_confidence);
         create_control_board_msg(2,lcd_msg);
      } else {
         sprintf(lcd_msg,"%02d %.2f S %.2f  ", cur_loc_estimate, cur_loc_conf, turn_confidence);
         create_control_board_msg(2,lcd_msg);
      }

      usleep(10);
   }

   //test turning
   compute_turn_prob(&turn_confidence); //compute turn confidence

   //if currently localized in a hallway and high turn confidence detected,
   //then execute a turn
   if ((cur_loc_estimate != -1) && (turn_confidence > .95) && (turn_in_progress == 0)) {
      //turn_dir = get_next_turn_dir(cur_loc_estimate,29); //next turn direction
      turn_dir = 0;

      update_goal_mutex.lock();
      //set_narrow_parameters(1); //use narrow hallway parameters when turning
      set_turn_parameters(); //use turn parameters when turning
      update_goal_mutex.unlock();
      execute_turn(); //execute left turn
      turn_in_progress = 1;
   }

   if (turn_in_progress) { //check if currently in a turn
      turn_progress_counter++;

      coord[0] = 11; //col 10
      coord[1] = 1; //row 1
      create_control_board_msg(1,coord);
      usleep(10);
      sprintf(lcd_msg,"Turn%d", turn_dir);
      create_control_board_msg(2,lcd_msg);
      usleep(10);

      //set a timeout for turning
      if (turn_progress_counter > 300) { //15 second timeout for a turn (15*20Hz = 300)
         //turn timeout
         update_goal_mutex.lock();
         action_client->cancelAllGoals(); //cancel the turn goal
         update_goal_mutex.unlock();
         goal_timer.start(); //start the goal update timer
         turn_in_progress = 0;
         turn_progress_counter = 0;
      }
      
      //turn is complete, restart the goal timer and set parameters
      bool turn_complete = false;
      update_goal_mutex.lock();
      if (action_client->getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
         turn_complete = true;
      }
      update_goal_mutex.unlock();

      if (turn_complete) {
         goal_timer.start(); //start the goal update timer
         turn_in_progress = 0;
         turn_progress_counter = 0;

         //update the route index
         current_route_index++;
         if (route_hallway[current_route_index] == -1) { //route completed, so start over
            current_route_index = 0;
         }

         //if the new hallway is narrow, set the driving parameters to closely follow the global path
         if (cur_loc_estimate != -1) {
            update_goal_mutex.lock();
            //int narrow = VAN(&gr, "narrow", cur_loc_estimate);
            /* if ((int)*loc >= 0 && (int)*loc < 25) { */
            /*    int narrow = VAN(&gr, "narrow", (int)*loc); */
            /*    set_narrow_parameters(narrow); */
            /* } */
            update_goal_mutex.unlock();
         }

         turn_start_counter = 50;
      }
   } else {
      coord[0] = 11; //col 10
      coord[1] = 1; //row 1
      create_control_board_msg(1,coord);
      usleep(10);
      sprintf(lcd_msg,"      ");
      create_control_board_msg(2,lcd_msg);
      usleep(10);

      if (turn_start_counter > 0) {
         turn_start_counter--;
      } else if (turn_start_counter == 0) {
         update_goal_mutex.lock();
         /* int narrow = VAN(&gr, "narrow", cur_loc_estimate); */
         /* set_narrow_parameters(narrow); */
         update_goal_mutex.unlock();

         turn_start_counter = -1;
      }
   }
}

void Navigation::write_text() {
   char str[100];

   if (turn[0] > .6) {
      sprintf (str, "turn: %.3f", (float) turn[0]);
      img_mutex.lock();
      cv::putText(new_image, //target image
               str, //text
               cv::Point(430, 30), //top-left position
               cv::FONT_HERSHEY_DUPLEX,
               .9,
               CV_RGB(0, 0, 255), //font color
               2);
      img_mutex.unlock();
   }

   //print the localization
   sprintf(str, "loc: %d (%.3f)", cur_loc, cur_loc_prob);

   if (cur_loc < 30) {
      /* img_mutex.lock(); */
      cv::putText(new_image, //target image
               str, //text
               cv::Point(430, 60), //top-left position
               cv::FONT_HERSHEY_DUPLEX,
               .8,
               cv::Scalar(255, 128, 0), //font color
               2);
      /* img_mutex.unlock(); */
   } else {
      /* img_mutex.lock(); */
      cv::putText(new_image, //target image
               str, //text
               cv::Point(430, 60), //top-left position
               cv::FONT_HERSHEY_DUPLEX,
               .8,
               cv::Scalar(0, 255, 0), //font color
               2);
      /* img_mutex.unlock(); */
   }

   //write the inference time
   sprintf (str, "nn (ms): %.2f", inference_time*100.0);
   /* img_mutex.lock(); */
   cv::putText(new_image, //target image
         str, //text
         cv::Point(430, 90), //top-left position
         cv::FONT_HERSHEY_DUPLEX,
         .8,
         cv::Scalar(0, 255, 0), //font color
         2);
   
   //write the heading
   sprintf (str, "heading: %.2f", actual_heading);
   cv::putText(new_image, //target image
         str, //text
         cv::Point(430, 120), //top-left position
         cv::FONT_HERSHEY_DUPLEX,
         .8,
         cv::Scalar(0, 255, 0), //font color
         2);
   /* img_mutex.unlock(); */
}

void Navigation::draw_goal() {
   /* img_mutex.lock(); */
   cv::circle( new_image,
         cv::Point(int(cur_goal_x), int(cur_goal_y)),
         3,
         cv::Scalar( 0, 255, 0),
         cv::FILLED,
         cv::LINE_8 );
   /* img_mutex.unlock(); */
}

void Navigation::compute_loc_prob() {
   //find the hallway with the highest probability
   float max_loc=0;
   int loc_index = 0;
   for (int i=0; i<NUM_LOC; i++) {
      if (loc[i] > max_loc) {
         max_loc = loc[i];
         cur_loc = i;
         cur_loc_prob = loc[i];
      }
   }

   //store the history of these hallway localizations in a circular buffer
   localization_index++;

   if (localization_index == LOCALIZATION_ARRAY_SIZE) {
      localization_index = 0;
   }

   localization_num[localization_index] = cur_loc;
   localization_value[localization_index] = cur_loc_prob; //localization index will point to most recent reading

   //store the position of this localization
   geometry_msgs::TransformStamped transformStamped;
   transformStamped = tfBuffer.lookupTransform("odom", "base_link", ros::Time(0));

   //store the pose of this localization
   localization_pose[localization_index].position.x = transformStamped.transform.translation.x;
   localization_pose[localization_index].position.y = transformStamped.transform.translation.y;
   localization_pose[localization_index].position.z = transformStamped.transform.translation.z;
   
   localization_pose[localization_index].orientation.w = transformStamped.transform.rotation.w;
   localization_pose[localization_index].orientation.x = transformStamped.transform.rotation.x;
   localization_pose[localization_index].orientation.y = transformStamped.transform.rotation.y;
   localization_pose[localization_index].orientation.z = transformStamped.transform.rotation.z;
   localization_turn_progress[localization_index]= turn_in_progress;

   heading_mutex.lock();
   localization_heading[localization_index] = actual_heading;
   heading_mutex.unlock();

   //tracking the turn probability from the neural network
   turn_index++;

   if (turn_index == TURN_ARRAY_SIZE) {
      turn_index = 0;
   }

   turn_tracking[turn_index] = turn[0]; //turn index will point to most recent reading

   //track the goal
   goal_cur_index++;

   if (goal_cur_index == GOAL_ARRAY_SIZE) {
      goal_cur_index = 0;
   }

   goal_array_x[goal_cur_index] = int(640 * goal[0]); //get the x of the goal
   goal_array_y[goal_cur_index] = int(360 * goal[1]); //get the y of the goal
   
}

#define HALLWAY_NUM 30
void Navigation::compute_localization(int* num, float* conf) {
   //localization from the neural network is updated at 20Hz
   float confidence_threshold = .85;
   int temp_index = localization_index;
   int hallway_count[HALLWAY_NUM];
   float hallway_confidence[HALLWAY_NUM];
   float overall_confidence;
   int max_count = 0;
   int max_estimate;
   int estimate;

   for (int i=0; i<HALLWAY_NUM; i++) {
      hallway_count[i] = 0;
      hallway_confidence[i] = 0;
   }

   for (int i=0; i<8; i++) {
      estimate = localization_num[temp_index];
      hallway_confidence[estimate] += localization_value[temp_index]; //sum up the confidence values
      hallway_count[estimate]++;
      temp_index--;

      if (temp_index < 0) {
         temp_index += LOCALIZATION_ARRAY_SIZE;
      }
   }

   //find the max number of hallway id's that appear
   for (int i=0; i<HALLWAY_NUM; i++) {
      if (hallway_count[i] > max_count) {
         max_count = hallway_count[i];
         max_estimate = i;
      }
   }

   overall_confidence = hallway_confidence[max_estimate]/(float)hallway_count[max_estimate];

   if ((max_count > 5) && (overall_confidence > confidence_threshold)) {
      *num = max_estimate;
      *conf = overall_confidence;
   } else {
      *num = -1;
      *conf = 0;
   }
}

int Navigation::compute_turn_prob(double* confidence) {
   //turn probability from the neural network is updated at 20Hz
   double avg_threshold = 0;
   float confidence_threshold = .90;
   int temp_index = turn_index;
   int above_threshold = 0;
   int num_samples = 4; //number of samples to read in a row

   for (int i=0; i<num_samples; i++) {
      if (turn_tracking[temp_index] > confidence_threshold) {
         above_threshold++;
      }

      avg_threshold += turn_tracking[temp_index];

      temp_index -= 1;

      if (temp_index < 0) {
         temp_index += TURN_ARRAY_SIZE;
      }
   }

   *confidence = avg_threshold / num_samples;

   if (above_threshold > (num_samples-1))
      return 1; //a turn
   else
      return 0;
}

void Navigation::compute_farthest(float* coord) {
   float highest_val = 1.0;
   int highest_index;
   float max_distance = 0;

   for (int i=0; i<NUM_GROUND; i++) {
      float x = i*8;
      float y = ground[i] * 360;

      //compute the distance accounting for the aspect ratio of 640/360 = 1.77
      float distance = sqrt(pow(320-x,2) + pow(1.77*(360-y),2));

      if (distance > max_distance) {
         max_distance = distance;
         highest_index = i;
      }

      //find the highest point in the image
      // if (ground[i] < highest_val) {
      //    highest_val = ground[i];
      //    highest_index = i;
      // }
   }

   coord[0] = highest_index*8;
   coord[1] = ground[highest_index] * 360;
}

void Navigation::connect_boundary() {
   for (int i=0; i<NUM_GROUND-1; i++) {
      int x1 = i*8;
      int y1 = ground[i]*360;
      int x2 = (i+1)*8;
      int y2 = ground[i+1]*360;
      /* img_mutex.lock(); */
      cv::line(new_image, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 0, 255), 2);
      /* img_mutex.unlock(); */
   }
}

void Navigation::draw_lines()
{
   cv::Point front_left_boundary = cv::Point(LEFT_OBSTACLE_X, LEFT_OBSTACLE_Y);
   cv::Point front_right_boundary = cv::Point(RIGHT_OBSTACLE_X, RIGHT_OBSTACLE_Y);

   // 3 foot lines in front of robot
   cv::line(new_image, cv::Point(212, 359), front_left_boundary, cv::Scalar(0, 255, 255), 2); //left trapezoid side

   // 3 foot right trapezoid side
   cv::line(new_image, cv::Point(428, 359), front_right_boundary, cv::Scalar(0, 255, 255), 2);

   // 2 foot lines in front of robot
   cv::line(new_image, cv::Point(212, 359), cv::Point(241, 275), cv::Scalar(255, 0, 0), 4); //left trapezoid side

   // 2 foot right trapezoid side
   cv::line(new_image, cv::Point(428, 359), cv::Point(399, 275), cv::Scalar(255, 0, 0), 4);

   // vertical line down middle
   // cv::line(new_image, cv::Point(320, 0), cv::Point(320, 359), cv::Scalar(255, 255, 0), 1);
}

int main(int argc, char** argv) {
  int c;

  while(1) {
     static struct option long_options[] =
     {
        /* These options set a flag. */
        //{"load",   no_argument,      &verbose_flag, 0},
        {"sim",   no_argument, &sim_mode, 1},
        {"nodebug", no_argument, &debug_mode, 0},
        /* These options don’t set a flag.
           We distinguish them by their indices. */
        {"accelerator", required_argument, 0, 'a'},
        {"build",   required_argument, 0, 'b'},
        {"device",  required_argument, 0, 'd'},
        {"width",   required_argument, 0, 'w'},
        {"height",  required_argument, 0, 'h'},
        {"load",    required_argument, 0, 'l'},
        {0, 0, 0, 0}
     };
     /* getopt_long stores the option index here. */
     int option_index = 0;

     c = getopt_long (argc, argv, "a:b:d:w:h:l:",
           long_options, &option_index);

     /* Detect the end of the options. */
     if (c == -1)
        break;
  }

  ros::init(argc, argv, "navigation_node");

  Navigation nav_node;
  nav_node.graph_init();
  /* ros::shutdown(); //exit */

  /* MoveBaseClient ac("move_base", true); */
  
  /* nav_node.set_action_client(&ac); */

  //wait for the action server to come up
  /* while(!ac.waitForServer(ros::Duration(5.0))){ */
  /*    ROS_INFO("Waiting for the move_base action server to come up"); */
  /* } */

  //since the goal callback is part of the nav_node object, 
  //bind the callback using boost:bind and boost:function
  boost::function<void(const ros::TimerEvent&)> goal_callback;
  goal_callback=boost::bind(&Navigation::update_goal_callback,&nav_node,_1);

  //run the goal update function every 3 seconds
  goal_timer = h->createTimer(ros::Duration(2.0), goal_callback); 

  ROS_INFO("Starting navigation");

  ros::spin();
  ros::waitForShutdown();
}
