#ifndef ENCODER_H
#define ENCODER_H

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>

class OdometryPublisher {
  int _PreviousLeftEncoderCounts, _PreviousRightEncoderCounts;
  ros::Time last_time, last_enc_time;
  double x,y,th;
  double MAX_ABS_LINEAR_SPEED, MAX_ABS_ANGULAR_SPEED, TICKS_PER_METER, BASE_WIDTH, ACC_LIM;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  //left_integral = [x for x in range(5)]
  //right_integral = [x for x in range(5)]
  int left_counter, right_counter, left_pwm, right_pwm;
  ros::Time last_set_speed_time;
  int last_left_error, last_right_error;
  int vl, vr;
  int last_enc_left, last_enc_right;
  double last_left_vel, last_right_vel;
  double left_tick_vel, right_tick_vel;
  double cur_x=0, cur_y=0, cur_theta=0;

  public:
    OdometryPublisher(); 
    void odometryCallBack(const std_msgs::Int32MultiArray::ConstPtr& msg);
    double normalize_angle(double angle);
    void publish_odometry_message(void); //publish a new Odometry message
    void update(int enc_left, int enc_right, double* vel_x, double* vel_theta);
    void update_publish_cb(const std_msgs::Int32MultiArray::ConstPtr& enc_msg); //called when a new encoder messaged received from the Arduino
};

#endif
