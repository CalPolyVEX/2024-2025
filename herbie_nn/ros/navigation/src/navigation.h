#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/ByteMultiArray.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>

#include <ros/console.h>
#include <boost/thread.hpp>
#include <iostream>
#include <cmath>
#include <queue>

#define INTEGRAL_ARRAY_SIZE 5
#define RECEIVE_PACKET_SIZE 13

struct packet {
   int size;
   unsigned char data[24];
};

class Navigation {
  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  tf2_ros::TransformBroadcaster odom_broadcaster;
  boost::mutex actual_vel_mutex;
  boost::mutex last_set_speed_time_mutex;
  boost::mutex desired_vel_mutex;
  boost::mutex setmotor_mutex;
  boost::mutex update_encoder_mutex;
  boost::mutex planner_mutex;
  boost::mutex herbie_board_queue_mutex;
  std::queue<struct packet> board_queue; //queue of messages to send to the Herbie control board

  int rtabmap_started = 0;
  int left_counter=0, right_counter=0;
  ros::Time last_set_speed_time;
  int last_left_error=0, last_right_error=0;
  int last_enc_left=0, last_enc_right=0; //last encoder counts
  double last_left_vel=0, last_right_vel=0; //last wheel velocities
  double left_tick_vel=0, right_tick_vel=0; //current wheel velocities
  double cur_x=0, cur_y=0, cur_theta=0;
  int desired_vl=0, desired_vr=0; //desired wheel velocities
  int cur_left_motor=0, cur_right_motor=0; //current motor command
  int update_encoders=1, stop=0, planner=0;
  int loop_closure = 0, proximity = 0;
  double left_integral[INTEGRAL_ARRAY_SIZE];
  double right_integral[INTEGRAL_ARRAY_SIZE];
  int debug_odometry = 0;
  int current_counter = 0;

  public:
    Navigation(); 

};

#endif
