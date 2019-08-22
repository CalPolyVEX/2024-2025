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
#include <std_msgs/String.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <boost/thread.hpp>
#include "bno055.h"
#include <rtabmap_ros/Info.h>

using namespace std;

ros::NodeHandle *nh;
ros::Subscriber rtabmap_info_sub;
ros::Publisher pub_, loop_closure_pub;

bno055::bno055() : tf_listener_(tf_buffer_) {
  bno055I2CBus = 1 ;       // Default Gen2 I2C bus for Jetson TK1, J3A1:18 & J3A1:20
  bno055I2CAddress = 0x70; // Defaults to 0x70 for bno055; jumper settable
  last_set_speed_time = ros::Time::now();

  //publish Odometry messages to this topic
  pub_ = nh->advertise<nav_msgs::Odometry>("/bno055_reading", 1);

  ROS_INFO("Connecting to roboclaw");

}

bool bno055::openbno055()
{
   char fileNameBuffer[32];
   sprintf(fileNameBuffer,"/dev/i2c-%d", bno055I2CBus);
   bno055I2CFileDescriptor = open(fileNameBuffer, O_RDWR);
   if (bno055I2CFileDescriptor < 0) {
       // Could not open the file
      error = errno ;
      return false ;
   }
   if (ioctl(bno055I2CFileDescriptor, I2C_SLAVE, bno055I2CAddress) < 0) {
       // Could not open the device on the bus
       error = errno ;
       return false ;
   }
   return true ;
}

void bno055::closebno055()
{
   if (bno055I2CFileDescriptor > 0) {
       close(bno055I2CFileDescriptor);
       // WARNING - This is not quite right, need to check for error first
       bno055I2CFileDescriptor = -1 ;
   }
}

void bno055::run(const ros::TimerEvent& ev) {
  ros::Time last_set_speed_time2;
  last_set_speed_time2 = ev.current_real;
}

void bno055::run_loop() {
  ros::Rate loop_rate(10);

  while (ros::ok())
  {
    std_msgs::String msg;

    std::stringstream ss;
    ss << "hello world " << 0;
    msg.data = ss.str();

    ROS_INFO("%s", msg.data.c_str());

    pub_.publish(msg);

    ros::spinOnce();

    loop_rate.sleep();
  }
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "bno055_node");
  ros::NodeHandle n;
  nh = &n;

  bno055 odom_pub;

  odom_pub.run_loop();

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
