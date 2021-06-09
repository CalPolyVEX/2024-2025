//6/16/19 This is used to publish odometry messages when
//each encoder message arrives

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/ByteMultiArray.h>
#include <std_msgs/Float64MultiArray.h>
#include <ros/console.h>
#include <iostream>
#include <boost/thread.hpp>
#include "navigation.h"

using namespace std;

ros::NodeHandle *nh;
ros::Subscriber sub_, sub_cmd_vel, sub_planner_cmd_vel;
ros::Subscriber nn_data_sub;
ros::Subscriber control_board_sub;
ros::Publisher pub_, loop_closure_pub;
ros::Publisher twist_pub;

Navigation::Navigation() : tf_listener_(tf_buffer_) {
  last_set_speed_time = ros::Time::now();

  //publish Odometry messages to this topic
  pub_ = nh->advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

  //subscriber for neural network data
  nn_data_sub = nh->subscribe("/nn_data", 1, &Navigation::cmd_vel_callback, this);
  
  ROS_INFO("Connecting to Herbie control board");
//   nh->param<std::string>("dev1", dev_name, "/dev/herbie");
//   nh->param<int>("baud1", baud_rate, 230400);
//   nh->param<int>("address1", address, 128);
//   nh->param<double>("max_abs_linear_speed1", MAX_ABS_LINEAR_SPEED, 1.0);
//   nh->param<double>("max_abs_angular_speed1", MAX_ABS_ANGULAR_SPEED, 2.0);
//   nh->param<double>("ticks_per_meter1", TICKS_PER_METER, 4467);
//   nh->param<double>("base_width1", BASE_WIDTH, 0.280988);
//   nh->param<double>("acc_lim1", ACC_LIM, 0.1);

  nh->setParam("/autonomous_mode", false);

  left_counter = 0;
  right_counter = 0;
}

void Navigation::cmd_vel_callback(const std_msgs::Float64MultiArray::ConstPtr& nn) {


}

int main(int argc, char** argv) {
  ros::init(argc, argv, "navigation_node");
  ros::NodeHandle n;
  nh = &n;

  Navigation n_pub;

  //since the diagnostics callback is part of the odom_pub object, 
  //bind the callback using boost:bind and boost:function
//   boost::function<void(const ros::TimerEvent&)> diag_callback;
//   diag_callback=boost::bind(&OdometryPublisher::safety_callback,&odom_pub,_1);

  ROS_INFO("Starting motor drive");

  ros::spin();
  ros::waitForShutdown();
}
