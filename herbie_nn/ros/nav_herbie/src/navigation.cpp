//This file contains the navigation node

#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <image_transport/image_transport.h>
#include <getopt.h>
#include "navigation.h"

/* #include <lemon/list_graph.h> */

using namespace std;

#define NUM_GROUND 80
#define NUM_LOC 64
#define NUM_TURN 1
#define NUM_GOAL 2
#define GOAL_AVG_COUNT 4

#define LEFT_OBSTACLE_X 256
#define LEFT_OBSTACLE_Y 229
#define RIGHT_OBSTACLE_X 384
#define RIGHT_OBSTACLE_Y 229

#define STOP_OBSTACLE_Y 295 

#define MAX_LINEAR .6
#define MAX_ANGULAR .8

int sim_mode = 0;
int debug_mode = 1;
ros::NodeHandle* h;
ros::Timer diag_timer;

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
}

void Navigation::img_callback(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImageConstPtr cv_ptr;

    try {
      cv_ptr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::BGR8);
    } catch (cv_bridge::Exception& e) {
      ROS_ERROR("cv_bridge exception: %s", e.what());
    }
    
   //resize image to IMG_HEIGHT x IMG_WIDTH
   if (cv_ptr->image.rows == 360 && cv_ptr->image.cols == 640) {
      memcpy(new_image.data, cv_ptr->image.data, 640*360*3);
   } else {
      cv::resize(cv_ptr->image, new_image, cv::Size(640,360), CV_INTER_LINEAR);
   }
}

void Navigation::nn_data_callback(const std_msgs::Float64MultiArray::ConstPtr& nn_msg) {
   double turn_confidence;

   ground = (double*) &(nn_msg->data[0]);
   loc = (double*) &(nn_msg->data[NUM_GROUND]);
   turn = (double*) &(nn_msg->data[NUM_GROUND + NUM_LOC]);
   goal = (double*) &(nn_msg->data[NUM_GROUND + NUM_TURN + NUM_LOC]);
   inference_time = (double) nn_msg->data[NUM_GROUND + NUM_TURN + NUM_LOC + 1];

   update_goal_transform();
   update_turn_transform();
   //draw the localization probability graph
   draw_loc_prob();

   //connect the boundary points with lines
   if (debug_mode) {
      //draw the ground boundary points
      for (int i=0; i<NUM_GROUND; i++) {
         cv::circle( new_image,
               cv::Point(i*8, int(ground[i] * 360)),
               2,
               cv::Scalar( 0, 0, 255 ),
               cv::FILLED,
               cv::LINE_8 );
      }

      connect_boundary();
      draw_lines();
      draw_goal();
      write_text();
   }

   //avoid_obstacles();

   //publish pointcloud
   publish_pointcloud();

   //publish message
   if(debug_mode) {
      sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
      image_pub_.publish(pub_msg);
   }

   turn_counter++;
   if (turn_counter > 2) {
      turn_counter = 0;

      char coord[2];
      char lcd_msg[32];
      coord[0] = 0; //col 0
      coord[1] = 0; //row 0
      create_control_board_msg(1,coord);
      usleep(5);

      //print localization
      if (compute_turn_prob(&turn_confidence) == 1) {
         sprintf(lcd_msg,"%02d %.2f T %.2f  ", cur_loc, cur_loc_prob, turn_confidence);
         create_control_board_msg(2,lcd_msg);
      } else {
         sprintf(lcd_msg,"%02d %.2f S %.2f  ", cur_loc, cur_loc_prob, turn_confidence);
         create_control_board_msg(2,lcd_msg);
      }

      usleep(5);
      coord[1] = 1;
      create_control_board_msg(1,coord); //move to second line on LCD
      usleep(5);
      sprintf(lcd_msg,"%03d,%03d", (int) (goal[0]*640), (int) (goal[1]*360));
      create_control_board_msg(2,lcd_msg);
      usleep(5);
   }

   //test turning
   int once = 0;
   if (cur_loc == 8 && turn_confidence > .95 && once == 0) {
      execute_turn();
      once = 1;
   }
}

void Navigation::write_text() {
   char str[100];

   if (turn[0] > .6) {
      sprintf (str, "turn: %.3f", (float) turn[0]);
      cv::putText(new_image, //target image
               str, //text
               cv::Point(430, 30), //top-left position
               cv::FONT_HERSHEY_DUPLEX,
               .9,
               CV_RGB(0, 0, 255), //font color
               2);
   }

   //print the localization
   sprintf(str, "loc: %d (%.3f)", cur_loc, cur_loc_prob);

   if (cur_loc < 30) {
      cv::putText(new_image, //target image
               str, //text
               cv::Point(430, 60), //top-left position
               cv::FONT_HERSHEY_DUPLEX,
               .8,
               cv::Scalar(255, 128, 0), //font color
               2);
   } else {
      cv::putText(new_image, //target image
               str, //text
               cv::Point(430, 60), //top-left position
               cv::FONT_HERSHEY_DUPLEX,
               .8,
               cv::Scalar(0, 255, 0), //font color
               2);
   }

   //write the inference time
   sprintf (str, "nn (ms): %.2f", inference_time*100.0);
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
}

void Navigation::draw_goal() {
   goal_cur_index = (goal_cur_index+1) % GOAL_AVG_COUNT;

   goal_x[goal_cur_index] = int(640 * goal[0]);
   goal_y[goal_cur_index] = int(360 * goal[1]);

   float total_x=0;
   float total_y=0;
   for (int i=0; i<GOAL_AVG_COUNT; i++) {
      total_x += goal_x[i];
      total_y += goal_y[i];
   }

   cur_goal_x = total_x / GOAL_AVG_COUNT;
   cur_goal_y = total_y / GOAL_AVG_COUNT;

   last_goal_x[goal_cur_index] = cur_goal_x;  //store the average goal positions
   last_goal_y[goal_cur_index] = cur_goal_y;

   //compute the variance of the goal 
   total_x = 0;
   float mean_x, variance;
   for(int i=0; i<GOAL_AVG_COUNT; i++) {
      total_x += last_goal_x[i];
   }
   mean_x = total_x / GOAL_AVG_COUNT;

   variance = 0;
   for(int i=0; i<GOAL_AVG_COUNT; i++) {
      variance += pow(mean_x-last_goal_x[i], 2);
   }
   variance /= GOAL_AVG_COUNT;

   /* std::cout << variance << std::endl; */

   if (variance > 70) { //moving goal, draw it as red
      cv::circle( new_image,
            cv::Point(int(cur_goal_x), int(cur_goal_y)),
            3,
            cv::Scalar( 0, 0, 255),
            cv::FILLED,
            cv::LINE_8 );
   } else {  //the goal is stable, draw it as green
      cv::circle( new_image,
            cv::Point(int(cur_goal_x), int(cur_goal_y)),
            3,
            cv::Scalar( 0, 255, 0),
            cv::FILLED,
            cv::LINE_8 );
   }
}

void Navigation::draw_loc_prob() {
   int base = 70;
   int top = 20;

   /* for (int i=0; i<NUM_LOC; i++) { */
   /*    int height = loc[i] * (base-top); */
   /*    if (i > 29) { */
   /*       cv::line(new_image, cv::Point(i*3, base-1), cv::Point(i*3, base-height-1), cv::Scalar(0, 255, 0), 3); */
   /*    } else { */
   /*       cv::line(new_image, cv::Point(i*3, base-1), cv::Point(i*3, base-height-1), cv::Scalar(255, 128, 0), 3); */
   /*    } */
   /* } */

   //find the highest localization node
   float max_loc=0;
   int loc_index = 0;
   for (int i=0; i<NUM_LOC; i++) {
      if (loc[i] > max_loc) {
         max_loc = loc[i];
         cur_loc = i;
         cur_loc_prob = loc[i];
      }
   }

   //tracking the localization values from the neural network
   localization_tracking[localization_index] = cur_loc;
   localization_value[localization_index] = cur_loc_prob;
   localization_index = (localization_index + 1); 

   if (localization_index == LOCALIZATION_ARRAY_SIZE) {
      localization_index = 0;
   }
   
   //tracking the turn probability from the neural network
   turn_tracking[turn_index] = turn[0];
   turn_index = (turn_index + 1); 

   if (turn_index == TURN_ARRAY_SIZE) {
      turn_index = 0;
   }
}

int Navigation::compute_localization() {
   //localization from the neural network is updated at 20Hz
   float confidence_threshold = .90;
   int temp_index = localization_index - 1;

   if (temp_index < 0) {
      temp_index += LOCALIZATION_ARRAY_SIZE;
   }

   int estimate = localization_tracking[temp_index];

   for (int i=0; i<4; i++) {
      if ((localization_tracking[temp_index] != estimate) || (localization_value[temp_index] < confidence_threshold)) {
         return -1;
      }

      temp_index -= 2;

      if (temp_index < 0) {
         temp_index += LOCALIZATION_ARRAY_SIZE;
      }
   }

   return estimate; //the localization estimate
}

int Navigation::compute_turn_prob(double* confidence) {
   //turn probability from the neural network is updated at 20Hz
   double avg_threshold = 0;
   float confidence_threshold = .90;
   int temp_index = turn_index - 1;
   int above_threshold = 0;

   if (temp_index < 0) {
      temp_index += TURN_ARRAY_SIZE;
   }

   for (int i=0; i<6; i++) {
      if (turn_tracking[temp_index] > confidence_threshold) {
         above_threshold++;
      }

      avg_threshold += turn_tracking[temp_index];

      temp_index -= 1;

      if (temp_index < 0) {
         temp_index += TURN_ARRAY_SIZE;
      }
   }

   *confidence = avg_threshold / 6.0;

   if (above_threshold > 4)
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

   //draw the farthest point
   // cv::circle( new_image,
   //    cv::Point(coord[0], coord[1]),
   //    4,
   //    cv::Scalar( 0, 255, 0),
   //    cv::FILLED,
   //    cv::LINE_8 );
}

void Navigation::connect_boundary() {
   for (int i=0; i<NUM_GROUND-1; i++) {
      int x1 = i*8;
      int y1 = ground[i]*360;
      int x2 = (i+1)*8;
      int y2 = ground[i+1]*360;
      cv::line(new_image, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 0, 255), 2);
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

void find_left_right_obstacle_coord(int y_coord, double* ground, int* left, int* right) {
   //starting at the y_coord, search to the left for an obstacle
   *left = 0;
   for (int i=40; i>0; i--) {
      if (int(ground[i] * 360) > y_coord)  {
         *left = i*8;
         break;
      }
   }

   //starting at the y_coord, search to the right for an obstacle
   *right = 639;
   for (int i=40; i<80; i++) {
      if (int(ground[i] * 360) > y_coord)  {
         *right = i*8;
         break;
      }
   }
}


float Navigation::compute_obstacle_force(int coord, int side) {
   /* if (1 == 0) { */
   /*    //compute Y force on left side///////////////////////////////////////////////////// */
   /*    float closest_left_distance = 10000; */
   /*    int min_left_x, min_left_y, min_right_x, min_right_y; */
   /*    int left_start_x = LEFT_OBSTACLE_X - (8*(num_obstacle_points-1)); */
   /*    int blocking_left = 0; */
   /*    for (int i=left_start_x; i <= (LEFT_OBSTACLE_X+16); i+=8) { */
   /*       int x1 = i; */
   /*       int y1 = ground[i/8]*360; */
   /*       float distance = sqrt(pow((x1-LEFT_OBSTACLE_X), 2) + pow((y1-LEFT_OBSTACLE_Y), 2)); */

   /*       if (distance < closest_left_distance) { //find the closest distance */
   /*          min_left_x = x1; */
   /*          min_left_y = y1; */
   /*          closest_left_distance = distance; */
   /*       } */

   /*       /1* if (y1 > LEFT_OBSTACLE_Y) { //obstacle too close *1/ */
   /*       /1*    blocking_left = 1; *1/ */
   /*       /1* } *1/ */

   /*    } */
   /*    cv::line(new_image, cv::Point(min_left_x, min_left_y), cv::Point(LEFT_OBSTACLE_X, LEFT_OBSTACLE_Y), cv::Scalar(0, 0, 255), 2); */

   /*    float left_mean = 0; */
   /*    float left_sd = 50; */
   /*    float ang_force_left = (1.0 / (left_sd * sqrt(2 * 3.14159))) * exp(-0.5 * pow((closest_left_distance-left_mean)/left_sd, 2)); */
   /*    ang_force_left *= -35.0; */

   /*    //if there is a blocking obstacle on the left side */
   /*    if (blocking_left) { */
   /*       ang_force_left = -2.0; */
   /*    } */

   /*    //compute Y force on right side/////////////////////////////////////////////////// */
   /*    float closest_right_distance = 10000; */
   /*    int right_start_x = RIGHT_OBSTACLE_X + (8*(num_obstacle_points-1)); */
   /*    int blocking_right = 0; */
   /*    for (int i=right_start_x; i >= (RIGHT_OBSTACLE_X-16); i-=8) { */
   /*       int x1 = i; */
   /*       int y1 = ground[i/8]*360; */
   /*       float distance = sqrt(pow((x1-RIGHT_OBSTACLE_X), 2) + pow((y1-RIGHT_OBSTACLE_Y), 2)); */

   /*       if (distance < closest_right_distance) { */
   /*          min_right_x = x1; */
   /*          min_right_y = y1; */
   /*          closest_right_distance = distance; */
   /*       } */

   /*       /1* if (y1 > RIGHT_OBSTACLE_Y) { //obstacle too close *1/ */
   /*       /1*    blocking_right = 1; *1/ */
   /*       /1* } *1/ */
   /*    } */
   /*    cv::line(new_image, cv::Point(min_right_x, min_right_y), cv::Point(RIGHT_OBSTACLE_X, RIGHT_OBSTACLE_Y), cv::Scalar(0, 0, 255), 2); */

   /*    float right_mean = 0; */
   /*    float right_sd = 50; */
   /*    float ang_force_right = (1.0 / (right_sd * sqrt(2 * 3.14159))) * exp(-0.5 * pow((closest_right_distance-right_mean)/right_sd, 2)); */
   /*    ang_force_right *= 35.0; */

   /*    //if there is a blocking obstacle on the left side */
   /*    if (blocking_right) { */
   /*       ang_force_right = 2.0; */
   /*    } */

   /*    return ang_force_left + ang_force_right; */ 
   /* } */

   //compute force
   float gauss_max = 1.5;
   float sd = 55;
   int distance_threshold = 110; //any closer than this distance from center, then max force is applied
   int center = 320;

   if (side == 1) { //right side
      int b = center + distance_threshold;

      if (coord < b) {
         return gauss_max;
      } else {
         float g = gauss_max * exp( - (pow(coord-b,2) / (2.0 * pow(sd,2))) ) ;
         return g;
      }
   } else {
      int b = center - distance_threshold;

      if (coord > b) {
         return gauss_max;
      } else {
         float g = gauss_max * exp( - (pow(coord-b,2) / (2.0 * pow(sd,2))) );
         return g;
      }
   }

   return 0;
}

void Navigation::avoid_obstacles()
{
   geometry_msgs::Twist t_cmd;

   //compute linear velocity attraction to goal point
   float goal_velocity = .005 * (LEFT_OBSTACLE_Y - cur_goal_y);

   if (goal_velocity > MAX_LINEAR) {
      goal_velocity = MAX_LINEAR;
   } else if (goal_velocity < -MAX_LINEAR) {
      goal_velocity = -MAX_LINEAR;
   }

   //compute forward distance to obstacle
   int boundary_index = LEFT_OBSTACLE_X / 8;
   int min_forward_distance = 360;

   //for all the points between the top of the trapezoid
   while ((boundary_index*8) >= LEFT_OBSTACLE_X && (boundary_index*8) <= RIGHT_OBSTACLE_X) {
      int y_ground = ground[boundary_index]*360;

      if (y_ground > LEFT_OBSTACLE_Y) {
         y_ground = LEFT_OBSTACLE_Y;
      }
      
      //get the distance from obstacle to the Y coordinate of the trapezoid
      int cur_distance = LEFT_OBSTACLE_Y - y_ground; 

      if (cur_distance < min_forward_distance) {
         min_forward_distance = cur_distance;
      }

      cv::circle(new_image,
                 cv::Point(int(boundary_index*8), int(LEFT_OBSTACLE_Y)),
                 3,
                 cv::Scalar(0, 255, 255),
                 cv::FILLED,
                 cv::LINE_8);
      boundary_index++;
   }

   //compute repulsive linear velocity - any obstacles will generate a negative
   //velocity force
   float mean = 0;
   float sd = 6;
   float obst_linear;

   if (min_forward_distance < 0) {
      min_forward_distance = 0.0;   
   }
   
   //the obstacle repulsive force is expressed as a Gaussian function
   obst_linear = (1.0 / (sd * sqrt(2 * 3.14159))) * exp(-0.5 * pow((min_forward_distance-mean)/sd, 2));
   obst_linear *= 4.0;

   /* cout << setprecision(3) << "linear_vel: " << goal_velocity; */
   // cout << " goal_distance: " << min_forward_distance;
   /* cout << setprecision(3) << " obst_linear: " << obst_linear; */

   //set the linear velocity to 0 if there is an obstacle
   int obstacle_stop = 0;
   for(int i=30; i<50; i++) { //look through ground boundary points 30 to 49
      int ground_y = ground[i]*360;

      if (ground_y > STOP_OBSTACLE_Y) {
         obstacle_stop = 1;
         break;
      }
   }
   
   //set the linear velocity
   if (obstacle_stop == 0) { //if there is an obstacle, come to a full stop
      t_cmd.linear.x = goal_velocity - obst_linear;
   } else {
      t_cmd.linear.x = 0;
   }

   if (t_cmd.linear.x < 0) {
      t_cmd.linear.x = 0;
   }

   /* t_cmd.linear.x = 0;  //FIXME do not move the robot forward */

   //compute angular velocity attraction to goal point
   float ang_velocity = .020 * (320.0 - cur_goal_x);
   float ang_max = 4.0;
   if (ang_velocity < -ang_max) { //limit the maximum of the angular velocity
      ang_velocity = -ang_max;
   } else if (ang_velocity > ang_max) {
      ang_velocity = ang_max;
   }
   /* cout << setprecision(3) << "  ang_vel: " << ang_velocity << endl; */

   t_cmd.angular.z = ang_velocity;

   int num_obstacle_points = 10;

   /* std::cout << "ang left: " << ang_force_left << "  ang right: " << ang_force_right << std::endl; */

   int left,right;

   int y_coord = 280;
   cv::line(new_image, cv::Point(310, y_coord), cv::Point(340, y_coord), cv::Scalar(0, 255, 0), 3);
   find_left_right_obstacle_coord(y_coord, ground, &left, &right);
   /* std::cout << "coord left: " << left << "  coord right: " << right << std::endl; */

   float ang_force_left = -compute_obstacle_force(left, 0);
   float ang_force_right = compute_obstacle_force(right, 1);

   /* std::cout << "\tleft force: " << ang_force_left << std::endl; */
   /* std::cout << "\tright force: " << ang_force_right << std::endl; */

   t_cmd.angular.z += ang_force_left + ang_force_right;

   //twist_pub_.publish(t_cmd);
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

  /* if (argc == 2 && strcmp(argv[1], "-sim") == 0) { */
  /*    sim_mode = 1; */
  /* } else { */
  /*    sim_mode = 0; */
  /* } */

  Navigation nav_node;
  nav_node.graph_init();
  ros::shutdown(); //exit

  MoveBaseClient ac("move_base", true);
  
  nav_node.set_action_client(&ac);

  //wait for the action server to come up
  while(!ac.waitForServer(ros::Duration(5.0))){
     ROS_INFO("Waiting for the move_base action server to come up");
  }

  //since the goal callback is part of the nav_node object, 
  //bind the callback using boost:bind and boost:function
  boost::function<void(const ros::TimerEvent&)> goal_callback;
  goal_callback=boost::bind(&Navigation::update_goal_callback,&nav_node,_1);

  //run the goal update function every 3 seconds
  diag_timer = h->createTimer(ros::Duration(3.0), goal_callback); 

  ROS_INFO("Starting navigation");

  ros::spin();
  ros::waitForShutdown();
}
