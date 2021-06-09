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
#include <ros/console.h>
#include <iostream>
#include <boost/thread.hpp>
#include "navigation.h"

using namespace std;

ros::NodeHandle *nh;
ros::Subscriber sub_, sub_cmd_vel, sub_planner_cmd_vel;
ros::Subscriber sub_stop;
ros::Subscriber control_board_sub;
ros::Publisher pub_, loop_closure_pub;
ros::Publisher twist_pub;

Navigation::Navigation() : tf_listener_(tf_buffer_) {
  last_set_speed_time = ros::Time::now();

  actual_vel_mutex.unlock();
  last_set_speed_time_mutex.unlock();
  desired_vel_mutex.unlock();
  update_encoder_mutex.unlock();
  herbie_board_queue_mutex.unlock();
  
  //encoder Int32MultiArray messages are received from the Arduino on this topic
  //sub_ = nh->subscribe("/encoder_service", 1, &OdometryPublisher::encoder_message_callback, this);

  //publish Odometry messages to this topic
  pub_ = nh->advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

  //publish Twist messages to this topic
//   twist_pub = nh->advertise<geometry_msgs::TwistWithCovarianceStamped>("/roboclaw_twist", 1);

  //publish Odometry messages to this topic
//   loop_closure_pub = nh->advertise<std_msgs::Int32MultiArray>("/update_loop_closure_lcd", 1);

  //listen for Twist messages on /cmd_vel
//   sub_cmd_vel = nh->subscribe("/cmd_vel", 2, &OdometryPublisher::cmd_vel_callback, this);
  
  //listen for Twist messages on /cmd_vel
//   sub_planner_cmd_vel = nh->subscribe("/planner/cmd_vel", 2, &OdometryPublisher::planner_cmd_vel_callback, this);

  //listen for Empty messages on /robot_stop
//   sub_stop = nh->subscribe("/robot_stop", 2, &OdometryPublisher::stop_toggle_callback, this);

  //listen for messages to send to the control board
//   control_board_sub = nh->subscribe("/control_board", 5, &OdometryPublisher::control_board_callback, this);

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

//   if (address > 0x87 || address < 0x80) {
//     ROS_INFO("Address out of range");
//   }

  for (int i=0;i<INTEGRAL_ARRAY_SIZE;i++) {
    left_integral[i] = 0;
    right_integral[i] = 0;
  }

  left_counter = 0;
  right_counter = 0;
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
