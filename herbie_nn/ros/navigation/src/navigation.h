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

  double* ground;
  double* loc;
  double* goal;

  public:
    Navigation(); 
    void nn_data_callback(const std_msgs::Float64MultiArray::ConstPtr& nn_msg);
    void img_callback(const sensor_msgs::ImageConstPtr& msg);
};

#endif
