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
#include <boost/thread.hpp>
#include <iostream>
#include <rtabmap_ros/Info.h>

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
  boost::mutex update_encoder_mutex;
  boost::mutex planner_mutex;
  serial::Serial *my_serial;

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
  int update_encoders=1, stop=1, planner=0;
  int loop_closure = 0, proximity = 0;
  double left_integral[INTEGRAL_ARRAY_SIZE];
  double right_integral[INTEGRAL_ARRAY_SIZE];

  public:
    OdometryPublisher(); 
    void publish_odometry_message(double vx, double vth); //publish a new Odometry message
    void encoder_message_callback(const std_msgs::Int32MultiArray::ConstPtr& enc_msg);
    void cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist);
    void planner_cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist);
    void rtabmap_info_callback(const rtabmap_ros::Info::ConstPtr& info);
    void stop_toggle_callback(const std_msgs::Empty::ConstPtr&);
    void run(const ros::TimerEvent& ev);

    //motor control and odometry functions
    void run_pid();
    void compute_pid(double left_desired, double left_actual, double right_desired, double right_actual);
    void update_odometry(int enc_left, int enc_right, double* vel_x, double* vel_theta);
    double normalize_angle(double angle);

    //Roboclaw functions
    void setmotor(int duty_cyclel, int dutycycler);
    void read_version();
    void read_voltage(unsigned short* voltage);
    void read_motor_currents(unsigned short* left_current, unsigned short* right_current);
    void read_status(unsigned short* status);
};

#endif