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
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/Int8.h>
#include <std_msgs/Empty.h>
#include <geometry_msgs/Twist.h>
#include <image_transport/image_transport.h>
#include <opencv2/imgproc/imgproc.hpp>

#include <ros/console.h>
//#include <boost/thread.hpp>
#include <mutex>
#include <iostream>
#include <cmath>
#include <queue>
#include <map>
#include <list>
#include <cstdio>

#include <nav_msgs/GetPlan.h>

/* #include <lemon/smart_graph.h> */
#include <igraph.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

class Navigation {
  struct Arc
  {
     std::string sourceID;
     std::string targetID;
     double cost;
  };

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
  double turn_tracking[TURN_ARRAY_SIZE];
  int turn_index = 0;
  int turn_counter = 0;
  geometry_msgs::TransformStamped turn_transform_left[64];
  geometry_msgs::TransformStamped turn_transform_right[64];
  int turn_in_progress = 0;
  int turn_progress_counter = 0;
  move_base_msgs::MoveBaseGoal cur_goal;

  //variables for localization
  #define LOCALIZATION_ARRAY_SIZE 200
  int localization_tracking[LOCALIZATION_ARRAY_SIZE];
  int localization_value[LOCALIZATION_ARRAY_SIZE];
  int localization_index = 0;

  //variables for heading
  float heading_offset = 0;
  float last_heading = 0;
  float actual_heading = 0;
  int heading_counter = 0;

  //move base variables
  MoveBaseClient* a;
  tf2_ros::Buffer tfBuffer;
  tf2_ros::TransformListener* tfListener;
  std::string service_name = "move_base/make_plan";
  std::mutex update_goal_mutex;

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
    float compute_obstacle_force(int coord, int side);

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
    int compute_localization();
    int compute_turn_prob(double* confidence);
    int call_make_plan(double goal_x, double goal_y, double* pose_x, double* pose_y);  //call the planner to test if a plan is found
    void execute_turn(int hallway_num, int dir);
    void set_narrow_parameters(int narrow);
    void init_turn_transforms();
    double get_distance_to_goal();

    //control board functions
    void create_control_board_msg(int num, void* arg);
    unsigned short compute_crc(unsigned char* data, int len);
};

#endif