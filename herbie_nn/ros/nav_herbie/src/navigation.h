#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/Pose.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/Int8.h>
#include <std_msgs/Empty.h>
#include <geometry_msgs/Twist.h>
#include <image_transport/image_transport.h>
#include <opencv2/imgproc/imgproc.hpp>

#include <ros/console.h>
#include <mutex>
#include <iostream>
#include <cmath>
#include <queue>
#include <map>
#include <list>
#include <cstdio>
#include <chrono>
#include <thread>
#include <atomic>

#include <nav_msgs/GetPlan.h>

#include <igraph.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

class Navigation {
  ros::NodeHandle nh;
  image_transport::ImageTransport it;

  ros::Subscriber nn_data_sub;
  ros::Subscriber odom_data_sub;
  ros::Subscriber nav_goal_sub;
  ros::Subscriber autonomous_sub;
  image_transport::Subscriber img_sub;
  image_transport::Publisher image_pub_;
  ros::Publisher twist_pub_;
  ros::Publisher pointcloud_pub_;
  ros::Publisher control_board_pub_;

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

  //variables for graph
  igraph_t gr;
  std::vector<std::string> nodes;

  //variables for turn
  #define TURN_ARRAY_SIZE 50
  double turn_tracking[TURN_ARRAY_SIZE]; //stores the last TURN_ARRAY_SIZE neural network predictions
  int turn_index = 0;
  int turn_dir = 1;                      //0=left, 1=straight, 2=right
  int turn_counter = 0;
  geometry_msgs::TransformStamped turn_transform_left[64];
  geometry_msgs::TransformStamped turn_transform_right[64];
  int turn_in_progress = 0;              //true if a turn is currently in progress
  int turn_progress_counter = 0;         //used to determine turn timeout
  move_base_msgs::MoveBaseGoal cur_goal;
  std::thread *turn_transform_thread;
  int route_hallway[100];
  int route_turn[100];
  int current_route_index = 0;         //the index into the route hallway array

  //variables for localization
  #define LOCALIZATION_ARRAY_SIZE 1200 //approximately 60 seconds worth of localization predictions
  int localization_num[LOCALIZATION_ARRAY_SIZE];          //stores the history of hallway predictions
  float localization_value[LOCALIZATION_ARRAY_SIZE];      //stores the confidence of those predictions
  geometry_msgs::Pose localization_pose[LOCALIZATION_ARRAY_SIZE]; //poses of last prediction 
  float localization_heading[LOCALIZATION_ARRAY_SIZE];    //store the last headings
  int localization_turn_progress[LOCALIZATION_ARRAY_SIZE];//store turn in progress flag
  int localization_index = 0;                             //current index into the hallway pred. array
  int cur_loc_estimate = -1;
  std::mutex cur_loc_mutex;

  //variables for heading
  float heading_offset = 0;  //an offset used to handle wrap around of the heading
  float last_heading = 0;
  float actual_heading = 0;
  int heading_counter = 0;
  std::mutex heading_mutex;

  //move base variables
  MoveBaseClient* action_client;
  tf2_ros::Buffer tfBuffer;
  tf2_ros::TransformListener* tfListener;
  std::string service_name = "move_base/make_plan";
  std::mutex update_goal_mutex;  //lock this mutex when updating the goal
  std::mutex img_mutex;  //lock this mutex when updating the output image

  public:
    Navigation(); 
    void nn_data_callback(const std_msgs::Float64MultiArray::ConstPtr& nn_msg);
    void img_callback(const sensor_msgs::ImageConstPtr& msg);
    void compute_farthest(float* coord);
    void connect_boundary();
    void compute_loc_prob();
    void draw_goal();
    void write_text();
    void draw_lines();

    //graph functions
    void odom_callback(const nav_msgs::Odometry::ConstPtr& msg); 
    void convert_to_heading(float w, float x, float y, float z);
    void graph_init();
    void path_to_next_goal();
    int get_next_turn_dir(int start, int end);

    //move base functions
    void send_goal(const std_msgs::Empty::ConstPtr& msg);
    void set_action_client(MoveBaseClient* ac);
    void publish_pointcloud();
    void autonomous_mode_callback(const std_msgs::Int8::ConstPtr& msg);
    void update_goal_transform();
    void update_turn_transform();
    void update_goal_callback(const ros::TimerEvent& ev);
    void compute_localization(int* num, float* conf);
    int compute_turn_prob(double* confidence);
    int call_make_plan(double goal_x, double goal_y, double* pose_x, double* pose_y);  //call the planner to test if a plan is found
    void execute_turn();
    void set_narrow_parameters(int narrow);
    void init_turn_transforms();
    double get_distance_to_goal();
    void init_route();
    void turn_degrees(int degrees);
    void set_turn_entry(int hallway_num, double x, double y, double degrees);

    //control board functions
    void create_control_board_msg(int num, void* arg);
    unsigned short compute_crc(unsigned char* data, int len);

    //recovery
    void get_major_heading();
};

#endif
