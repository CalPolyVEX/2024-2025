#ifndef ENCODER_H
#define ENCODER_H

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>

#define INTEGRAL_ARRAY_SIZE 5

class OdometryPublisher {
  std::string dev_name;
  int baud_rate, address;
  ros::Time last_enc_time; //time of the last encoder reading
  double MAX_ABS_LINEAR_SPEED, MAX_ABS_ANGULAR_SPEED, TICKS_PER_METER, BASE_WIDTH, ACC_LIM;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  tf2_ros::TransformBroadcaster odom_broadcaster;
  boost::mutex actual_vel_mutex;
  boost::mutex last_set_speed_time_mutex;
  boost::mutex desired_vel_mutex;
  boost::mutex setmotor_mutex;
  serial::Serial *my_serial;

  double left_integral[INTEGRAL_ARRAY_SIZE];
  double right_integral[INTEGRAL_ARRAY_SIZE];
  int left_counter=0, right_counter=0, left_pwm, right_pwm;
  ros::Time last_set_speed_time;
  int last_left_error=0, last_right_error=0;
  int last_enc_left=0, last_enc_right=0; //last encoder counts
  double last_left_vel=0, last_right_vel=0; //last wheel velocities
  double left_tick_vel=0, right_tick_vel=0; //current wheel velocities
  double cur_x=0, cur_y=0, cur_theta=0;
  int desired_vl=0, desired_vr=0; //desired wheel velocities
  int cur_left_motor=0, cur_right_motor=0; //current motor command

  public:
    OdometryPublisher(); 
    void odometryCallBack(const std_msgs::Int32MultiArray::ConstPtr& msg);
    double normalize_angle(double angle);
    void publish_odometry_message(double vx, double vth); //publish a new Odometry message
    void update_odometry(int enc_left, int enc_right, double* vel_x, double* vel_theta);
    void encoder_message_callback(const std_msgs::Int32MultiArray::ConstPtr& enc_msg);
    void compute_pid(double left_desired, double left_actual, double right_desired, double right_actual, double* left_set_value, double* right_set_value);
    void cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist);
    void run(const ros::TimerEvent& ev);
    void setmotor(int motor_num, int duty_cycle);
    void run_pid(const ros::TimerEvent& e);
    void read_version();
};

#endif
