//6/16/19 This is used to publish odometry messages when
//each encoder message arrives

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <std_msgs/Float64MultiArray.h>
#include <geometry_msgs/Twist.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <image_transport/image_transport.h>
#include <ros/console.h>
#include <iostream>
#include <boost/thread.hpp>
#include <cmath>
#include "navigation.h"

#include <lemon/list_graph.h>

using namespace std;

#define NUM_GROUND 80
#define NUM_LOC 64
#define NUM_TURN 1
#define NUM_GOAL 2
#define GOAL_AVG_COUNT 3

#define LEFT_OBSTACLE_X 256
#define LEFT_OBSTACLE_Y 229
#define RIGHT_OBSTACLE_X 384
#define RIGHT_OBSTACLE_Y 229

#define STOP_OBSTACLE_Y 295 

#define MAX_LINEAR .6
#define MAX_ANGULAR .8

int sim_mode = 0;

Navigation::Navigation() : it(nh) {
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

  nh.setParam("/autonomous_mode", false);

  new_image = cv::Mat(360, 640, CV_8UC3);
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
   ground = (double*) &(nn_msg->data[0]);
   loc = (double*) &(nn_msg->data[NUM_GROUND]);
   turn = (double*) &(nn_msg->data[NUM_GROUND + NUM_LOC]);
   goal = (double*) &(nn_msg->data[NUM_GROUND + NUM_TURN + NUM_LOC]);
   inference_time = (double) nn_msg->data[NUM_GROUND + NUM_TURN + NUM_LOC + 1];

   //draw the ground boundary points
   for (int i=0; i<NUM_GROUND; i++) {
      cv::circle( new_image,
         cv::Point(i*8, int(ground[i] * 360)),
         2,
         cv::Scalar( 0, 0, 255 ),
         cv::FILLED,
         cv::LINE_8 );
   }

   //connect the boundary points with lines
   connect_boundary();
   draw_lines();
   avoid_obstacles();

   float coord[2];
   compute_farthest(coord);

   //draw the localization probability graph
   draw_loc_prob();

   draw_goal();

   write_text();

   //publish message
   sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
   image_pub_.publish(pub_msg);
}

void Navigation::write_text() {
   char str[100];

   if (turn[0] > .4) {
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

   if (variance > 50) {
      cv::circle( new_image,
            cv::Point(int(cur_goal_x), int(cur_goal_y)),
            3,
            cv::Scalar( 0, 0, 255),
            cv::FILLED,
            cv::LINE_8 );
   } else {
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

   for (int i=0; i<NUM_LOC; i++) {
      int height = loc[i] * (base-top);
      if (i > 29) {
         cv::line(new_image, cv::Point(i*3, base-1), cv::Point(i*3, base-height-1), cv::Scalar(0, 255, 0), 3);
      } else {
         cv::line(new_image, cv::Point(i*3, base-1), cv::Point(i*3, base-height-1), cv::Scalar(255, 128, 0), 3);
      }
   }

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

   //for all the point between the top of the trapezoid
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

   cout << setprecision(3) << "linear_vel: " << goal_velocity;
   // cout << " goal_distance: " << min_forward_distance;
   cout << setprecision(3) << " obst_linear: " << obst_linear;

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

   //compute angular velocity attraction to goal point
   float ang_velocity = .003 * (320.0 - cur_goal_x);
   cout << setprecision(3) << "  ang_vel: " << ang_velocity << endl;

   t_cmd.angular.z = ang_velocity;

   int num_obstacle_points = 10;

   //compute Y force on left side
   float closest_left_distance = 10000;
   int min_left_x, min_left_y, min_right_x, min_right_y;
   int left_start_x = LEFT_OBSTACLE_X - (8*(num_obstacle_points-1));
   for (int i=left_start_x; i <= (LEFT_OBSTACLE_X+16); i+=8) {
      // cv::circle(new_image,
      //            cv::Point(int(i), int(LEFT_OBSTACLE_Y)),
      //            3,
      //            cv::Scalar(0, 255, 255),
      //            cv::FILLED,
      //            cv::LINE_8);

      int x1 = i;
      int y1 = ground[i/8]*360;
      float distance = sqrt(pow((x1-LEFT_OBSTACLE_X), 2) + pow((y1-LEFT_OBSTACLE_Y), 2));

      if (distance < closest_left_distance) {
         min_left_x = x1;
         min_left_y = y1;
         closest_left_distance = distance;
      }
   }
   cv::line(new_image, cv::Point(min_left_x, min_left_y), cv::Point(LEFT_OBSTACLE_X, LEFT_OBSTACLE_Y), cv::Scalar(0, 0, 255), 2);

   float left_mean = 0;
   float left_sd = 40;
   float ang_force_left = (1.0 / (left_sd * sqrt(2 * 3.14159))) * exp(-0.5 * pow((closest_left_distance-left_mean)/left_sd, 2));
   ang_force_left *= -15.0;

   //compute Y force on right side
   float closest_right_distance = 10000;
   int right_start_x = RIGHT_OBSTACLE_X + (8*(num_obstacle_points-1));
   for (int i=right_start_x; i >= (RIGHT_OBSTACLE_X-16); i-=8) {
      // cv::circle(new_image,
      //            cv::Point(int(i), int(RIGHT_OBSTACLE_Y)),
      //            3,
      //            cv::Scalar(0, 255, 255),
      //            cv::FILLED,
      //            cv::LINE_8);

      int x1 = i;
      int y1 = ground[i/8]*360;
      float distance = sqrt(pow((x1-RIGHT_OBSTACLE_X), 2) + pow((y1-RIGHT_OBSTACLE_Y), 2));

      if (distance < closest_right_distance) {
         min_right_x = x1;
         min_right_y = y1;
         closest_right_distance = distance;
      }
   }
   cv::line(new_image, cv::Point(min_right_x, min_right_y), cv::Point(RIGHT_OBSTACLE_X, RIGHT_OBSTACLE_Y), cv::Scalar(0, 0, 255), 2);

   float right_mean = 0;
   float right_sd = 40;
   float ang_force_right = (1.0 / (right_sd * sqrt(2 * 3.14159))) * exp(-0.5 * pow((closest_right_distance-right_mean)/right_sd, 2));
   ang_force_right *= 15.0;

   std::cout << "ang left: " << ang_force_left << "  ang right: " << ang_force_right << std::endl;

   t_cmd.angular.x = ang_force_left + ang_force_right;
   t_cmd.angular.y = ang_force_right;

   twist_pub_.publish(t_cmd);
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "navigation_node");

  if (argc == 2 && strcmp(argv[1], "-sim") == 0) {
     sim_mode = 1;
  } else {
     sim_mode = 0;
  }

  Navigation nav_node;
  nav_node.graph_init();

  //since the diagnostics callback is part of the odom_pub object, 
  //bind the callback using boost:bind and boost:function
//   boost::function<void(const ros::TimerEvent&)> diag_callback;
//   diag_callback=boost::bind(&OdometryPublisher::safety_callback,&odom_pub,_1);

  ROS_INFO("Starting navigation");

  ros::spin();
  ros::waitForShutdown();
}
