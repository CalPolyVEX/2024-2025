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
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstdio>
#include <sys/ioctl.h>

#define INTEGRAL_ARRAY_SIZE 5

class bno055 {
  std::string dev_name;
  ros::Time last_enc_time; //time of the last encoder reading
  int bno055I2CBus; 
  int bno055I2CAddress;
  int error;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  tf2_ros::TransformBroadcaster odom_broadcaster;
  ros::Time last_set_speed_time;
  int bno055I2CFileDescriptor;

  public:
    bno055(); 
    void run_loop();
    bool openbno055();
    void closebno055();
    void run(const ros::TimerEvent& ev);
};

#endif
