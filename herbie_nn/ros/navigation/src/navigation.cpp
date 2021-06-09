//6/16/19 This is used to publish odometry messages when
//each encoder message arrives

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <std_msgs/Float64MultiArray.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <image_transport/image_transport.h>
#include <ros/console.h>
#include <iostream>
#include <boost/thread.hpp>
#include "navigation.h"

using namespace std;

Navigation::Navigation() : it(nh) {
  //subscriber for neural network data
  nn_data_sub = nh.subscribe("/nn_data", 1, &Navigation::nn_data_callback, this);
  
  //subscriber for 640x360 image
  image_transport::TransportHints hints("compressed");
  //image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &ObstacleDetection::run_network, this, hints);
  img_sub = it.subscribe("/see3cam_cu20/image_raw", 1, &Navigation::img_callback, this, hints);

//   nh->param<std::string>("dev1", dev_name, "/dev/herbie");
//   nh->param<int>("baud1", baud_rate, 230400);
//   nh->param<int>("address1", address, 128);
//   nh->param<double>("max_abs_linear_speed1", MAX_ABS_LINEAR_SPEED, 1.0);
//   nh->param<double>("max_abs_angular_speed1", MAX_ABS_ANGULAR_SPEED, 2.0);
//   nh->param<double>("ticks_per_meter1", TICKS_PER_METER, 4467);
//   nh->param<double>("base_width1", BASE_WIDTH, 0.280988);
//   nh->param<double>("acc_lim1", ACC_LIM, 0.1);

  nh.setParam("/autonomous_mode", false);
}

void Navigation::img_callback(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImageConstPtr cv_ptr;

    try {
      cv_ptr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::BGR8);
    } catch (cv_bridge::Exception& e) {
      ROS_ERROR("cv_bridge exception: %s", e.what());
    }
    
    cout << "received image" << endl;
}

void Navigation::nn_data_callback(const std_msgs::Float64MultiArray::ConstPtr& nn_msg) {
   ground = (double*) &(nn_msg->data[0]);
   loc = (double*) &(nn_msg->data[80]);
   goal = (double*) &(nn_msg->data[144]);

   for (int i=0; i<80; i++) {
      cout << ground[i];
   }
   cout << endl;
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "navigation_node");

  Navigation nav_node;

  //since the diagnostics callback is part of the odom_pub object, 
  //bind the callback using boost:bind and boost:function
//   boost::function<void(const ros::TimerEvent&)> diag_callback;
//   diag_callback=boost::bind(&OdometryPublisher::safety_callback,&odom_pub,_1);

  ROS_INFO("Starting navigation");

  ros::spin();
  ros::waitForShutdown();
}
