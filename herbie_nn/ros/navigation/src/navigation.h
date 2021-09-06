#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Float64MultiArray.h>
#include <image_transport/image_transport.h>
#include <opencv2/imgproc/imgproc.hpp>

#include <ros/console.h>
#include <boost/thread.hpp>
#include <iostream>
#include <cmath>
#include <queue>

#include <lemon/smart_graph.h>

class Navigation {
  struct Arc
  {
     std::string sourceID;
     std::string targetID;
     double cost;
  };

  ros::NodeHandle nh;
  image_transport::ImageTransport it;
  // boost::mutex actual_vel_mutex;

  ros::Subscriber nn_data_sub;
  ros::Subscriber odom_data_sub;
  image_transport::Subscriber img_sub;
  image_transport::Publisher image_pub_;
  ros::Publisher twist_pub_;

  double* ground;
  double* loc;
  double* turn;
  double* goal;
  double inference_time;
  cv::Mat new_image;
  int cur_loc;  //the index of the current location
  float cur_loc_prob; //the probability of the current location
  float goal_x[6];
  float goal_y[6];
  float last_goal_x[6];
  float last_goal_y[6];
  float cur_goal_x, cur_goal_y;
  int goal_cur_index = 0;
  float heading_offset = 0;
  float last_heading = 0;
  float actual_heading = 0;
  int heading_counter = 0;

  std::vector<std::string> nodes;

  public:
    Navigation(); 
    void nn_data_callback(const std_msgs::Float64MultiArray::ConstPtr& nn_msg);
    void img_callback(const sensor_msgs::ImageConstPtr& msg);
    void compute_farthest(float* coord);
    void connect_boundary();
    void draw_loc_prob();
    void draw_goal();
    void write_text();
    void draw_lines();
    void avoid_obstacles();
    void graph_init();
    void path_to_next_goal();
    void odom_callback(const nav_msgs::Odometry::ConstPtr& msg); 
    void convert_to_heading(float w, float x, float y, float z);
    float compute_obstacle_force(int coord, int side);
};

#endif
