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
using namespace std;

class EncoderOdom {
  int _PreviousLeftEncoderCounts, _PreviousRightEncoderCounts;
  ros::Time last_time;
  double x,y,th;
  double MAX_ABS_LINEAR_SPEED, MAX_ABS_ANGULAR_SPEED, TICKS_PER_METER, BASE_WIDTH, ACC_LIM;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

public:
  EncoderOdom();
  void odometryCallBack(const std_msgs::Int32MultiArray::ConstPtr& msg);
};

