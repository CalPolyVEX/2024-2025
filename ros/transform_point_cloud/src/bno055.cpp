//6/16/19 This is used to publish odometry messages when
//each encoder message arrives

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <boost/thread.hpp>
#include "bno055.h"
#include <rtabmap_ros/Info.h>

using namespace std;

ros::NodeHandle *nh;
ros::Subscriber sub_, sub_cmd_vel, sub_planner_cmd_vel;
ros::Subscriber sub_stop;
ros::Subscriber rtabmap_info_sub;
ros::Publisher pub_, loop_closure_pub;

bno055::bno055() : tf_listener_(tf_buffer_) {
  last_set_speed_time = ros::Time::now();

  //publish Odometry messages to this topic
  pub_ = nh->advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

  ROS_INFO("Connecting to roboclaw");

}

void bno055::run(const ros::TimerEvent& ev) {
  ros::Time last_set_speed_time2;
  last_set_speed_time2 = ev.current_real;
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "bno055_node");
  ros::NodeHandle n;
  nh = &n;

  bno055 odom_pub;

  //since the diagnostics callback is part of the odom_pub object, 
  //bind the callback using boost:bind and boost:function
  boost::function<void(const ros::TimerEvent&)> diag_callback;
  diag_callback=boost::bind(&bno055::run,&odom_pub,_1);

  ROS_INFO("Starting motor drive");
  //run diagnostics function at 5Hz (.2 seconds)
  ros::Timer diag_timer = nh->createTimer(ros::Duration(.5), diag_callback);

  ros::spin();
  ros::waitForShutdown();
}
