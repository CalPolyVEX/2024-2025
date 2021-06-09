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
#include <cmath>
#include "navigation.h"

using namespace std;

#define NUM_GROUND 80
#define NUM_LOC 64
#define NUM_GOAL 2

Navigation::Navigation() : it(nh) {
  //subscriber for neural network data
  nn_data_sub = nh.subscribe("/nn_data", 1, &Navigation::nn_data_callback, this);
  
  //subscriber for 640x360 image
  image_transport::TransportHints hints("compressed");
  img_sub = it.subscribe("/see3cam_cu20/image_raw", 1, &Navigation::img_callback, this, hints);

  image_pub_ = it.advertise("/image_converter/output_video", 1);

//   nh->param<double>("ticks_per_meter1", TICKS_PER_METER, 4467);

  nh.setParam("/autonomous_mode", false);
}

void Navigation::img_callback(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImageConstPtr cv_ptr;

    try {
      cv_ptr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::BGR8);
    } catch (cv_bridge::Exception& e) {
      ROS_ERROR("cv_bridge exception: %s", e.what());
    }
    
   //  cout << "received image" << endl;

   //resize image to IMG_HEIGHT x IMG_WIDTH
   cv::resize(cv_ptr->image, new_image, cv::Size(640,360), CV_INTER_LINEAR);
}

void Navigation::nn_data_callback(const std_msgs::Float64MultiArray::ConstPtr& nn_msg) {
   ground = (double*) &(nn_msg->data[0]);
   loc = (double*) &(nn_msg->data[NUM_GROUND]);
   goal = (double*) &(nn_msg->data[NUM_GROUND + NUM_LOC]);

   //draw the ground boundary points
   for (int i=0; i<NUM_GROUND; i++) {
      cv::circle( new_image,
         cv::Point(i*8, int(ground[i] * 360)),
         2,
         cv::Scalar( 0, 0, 255 ),
         cv::FILLED,
         cv::LINE_8 );
   }

   float coord[2];
   compute_farthest(coord);

   //draw the highest point
   cv::circle( new_image,
      cv::Point(coord[0], coord[1]),
      4,
      cv::Scalar( 0, 255, 0),
      cv::FILLED,
      cv::LINE_8 );

   // connect_boundary();
   draw_loc_prob();

   //publish message
   sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
   image_pub_.publish(pub_msg);
}

void Navigation::draw_loc_prob() {
   int base = 70;
   int top = 20;

   for (int i=0; i<NUM_LOC; i++) {
      int height = loc[i] * (base-top);
      if (i > 29) {
         cv::line(new_image, cv::Point(i*3, base-1), cv::Point(i*3, base-height-1), cv::Scalar(0, 255, 0), 2);
      } else {
         cv::line(new_image, cv::Point(i*3, base-1), cv::Point(i*3, base-height-1), cv::Scalar(255, 128, 0), 2);
      }
   }
}

void Navigation::compute_farthest(float* coord) {
   float highest_val = 1.0;
   int highest_index;
   float max_distance = 0;

   for (int i=0; i<NUM_GROUND; i++) {
      float x = i*8;
      float y = ground[i] * 360;

      float distance = sqrt(pow(320-x,2) + pow(1.77*(360-y),2));

      if (distance > max_distance) {
         max_distance = distance;
         highest_index = i;
      }
      // if (ground[i] < highest_val) {
      //    highest_val = ground[i];
      //    highest_index = i;
      // }
   }

   coord[0] = highest_index*8;
   coord[1] = ground[highest_index] * 360;
}

void Navigation::connect_boundary() {
   for (int i=0; i<NUM_GROUND-1; i++) {
      int x1 = i*8;
      int y1 = ground[i]*360;
      int x2 = (i+1)*8;
      int y2 = ground[i+1]*360;
      cv::line(new_image, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 255), 2);
   }
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
