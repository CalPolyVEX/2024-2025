#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Float64MultiArray.h>
#include <image_transport/image_transport.h>
#include <opencv2/imgproc/imgproc.hpp>

#include <ros/console.h>
#include <boost/thread.hpp>
#include <iostream>
#include <cmath>
#include <queue>

class Navigation {
  ros::NodeHandle nh;
  image_transport::ImageTransport it;
  // boost::mutex actual_vel_mutex;

  ros::Subscriber nn_data_sub;
  image_transport::Subscriber img_sub;
  image_transport::Publisher image_pub_;
  ros::Publisher twist_pub_;

  double* ground;
  double* loc;
  double* turn;
  double* goal;
  cv::Mat new_image;
  int cur_loc;  //the index of the current location
  float cur_loc_prob; //the probability of the current location
  float goal_x[4];
  float goal_y[4];
  float cur_goal_x, cur_goal_y;
  int goal_cur_index = 0;

  public:
    Navigation(); 
    void nn_data_callback(const std_msgs::Float64MultiArray::ConstPtr& nn_msg);
    void img_callback(const sensor_msgs::ImageConstPtr& msg);
    void compute_farthest(float* coord);
    void connect_boundary();
    void draw_loc_prob();
    void draw_goal();
    void write_text();
    void draw_lines();
    void avoid_obstacles();
};

#endif