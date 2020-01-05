#include <ros/ros.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Int8.h>
#include <actionlib/client/simple_action_client.h>
#include <iostream>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/Twist.h>
#include <cmath>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <dynamic_reconfigure/DoubleParameter.h>
#include <dynamic_reconfigure/Reconfigure.h>
#include <dynamic_reconfigure/Config.h>

using namespace std;

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;
ros::NodeHandle* nh;
tf2_ros::Buffer tfBuffer_;
tf2_ros::TransformListener* tfListener_;
boost::thread *check_movement_ptr;

struct goal {
  double x;
  double y;
  int id;
  int heading;
};

struct route {
  int waypoints[20];
  int heading[20];
  int pause[20]; //the amount of time to pause at that location (default is 0)
  int length;
};

class Navigation {
  float linear_x;
  float angular_z;
  ros::Subscriber goal_sub;
  ros::Subscriber sub_planner_cmd;
  MoveBaseClient* ac;
  int cur_goal_in_route = 0; //the current goal index in the route
  int current_route;
  int current_goal_index;
  int start_route = 0;
  boost::thread *workerThread;
  boost::thread *check_thread;
  boost::mutex route_mutex;
  boost::mutex planner_cmd_mutex;
  struct goal goals[40]; //database of all the goals
  struct route routes[10];

  public:
    Navigation();
    void enable_rtabmap_mapping(int enable);
    void init_action_client();
    void send_goal_callback(const std_msgs::Int8::ConstPtr& mesg);
    void planner_cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist);
    void set_action_client(MoveBaseClient* a);
    void set_heading(int degrees, double* w, double* x, double* y, double* z);
    double get_distance_from_goal();
    void fill_goal_info(move_base_msgs::MoveBaseGoal *goal, int goal_index, int heading);
    void route_thread();
    void check_distance_thread();
    void check_movement_thread();
};

/* Navigation::Navigation() : tfListener(tfBuffer) { */
Navigation::Navigation() {
  /* planner_cmd_vel_mutex.unlock(); */
  workerThread = new boost::thread(boost::bind(&Navigation::route_thread, this));
  check_thread = new boost::thread(boost::bind(&Navigation::check_distance_thread, this));
  check_movement_ptr = new boost::thread(boost::bind(&Navigation::check_movement_thread, this));

  //the /autonomous topic send a message if a goal is requested
  goal_sub = nh->subscribe("/autonomous",1,&Navigation::send_goal_callback,this); 

  //subscribe to the planner velocity messages
  sub_planner_cmd = nh->subscribe("/planner/cmd_vel", 1, &Navigation::planner_cmd_vel_callback, this);

  goals[0].x = .5;       //x of id#1 (Seng office)
  goals[0].y = 0;        //y of id#1

  goals[1].x = 14.4542;  //x of id#65 (men's bathroom)
  goals[1].y = 6.03422;  //y of id#65

  goals[2].x = 15.2227;  //x of id#369 (women's bathroom)
  goals[2].y = -11.9279; //y of id#369

  goals[3].x = -12.303691; //x of id#1315 (outside south quad, east entrance)
  goals[3].y = -13.27466; //y of id#1315

  goals[4].x = 3.96606;   //x of id#198 (Kurfess office)
  goals[4].y = .321492; //y of id#198

  goals[5].x = 4.731087;   //x of id#1315 (outside north quad, east entrance)
  goals[5].y = -12.982945; //y of id#1315

  goals[6].x = -13.2928; //x of id#1714 (outside south quad, west entrance)
  goals[6].y = 5.05869;  //y of id#1714

  goals[7].x = -14.6241; //x of id#855 (Phil's office)
  goals[7].y = -1.31144; //y of id#855
  
  goals[8].x = 4.17692;  //x of id#1529 (Ventura's office)
  goals[8].y = -6.37767; //y of id#1529

  goals[9].x = 3.84982;  //x of id#1151 (outside north quad, west entrance)
  goals[9].y = 6.40079; //y of id#1151

  //---------Dexter lawn goals------------
  goals[10].x = 0; //x of id#1151 (reference start in front of the Waypoint sign)
  goals[10].y = 0; //y of id#1151

  goals[11].x = -19.0; //x of id#1151 (in front of the bench)
  goals[11].y = -.2;  //y of id#1151

  goals[12].x = 1.17; //x of id#1151 (right of the sign)
  goals[12].y = -1.22; //y of id#1151

  goals[13].x = 3.64; //x of id#1151 (next to the sign)
  goals[13].y = -1.38; //y of id#1151

  goals[14].x = 6.79; //x of id#1151 (cross diagonal)
  goals[14].y = -3.61; //y of id#1151
  //---------End Dexter lawn goals------------

  //---------new building 14 goals------------
  float offset_x = .5;
  goals[15].x = .5 + offset_x;       //x of id#1 (Seng office)
  goals[15].y = 0;        //y of id#1

  goals[16].x = 14.4542+ offset_x;  //x of id#65 (men's bathroom)
  goals[16].y = 6.03422;  //y of id#65

  goals[17].x = 15.2227+ offset_x;  //x of id#369 (women's bathroom)
  goals[17].y = -11.9279; //y of id#369

  goals[18].x = -12.303691+ offset_x; //x of id#1315 (outside south quad, east entrance)
  goals[18].y = -13.27466; //y of id#1315

  goals[19].x = 3.96606+ offset_x;   //x of id#198 (Kurfess office)
  goals[19].y = .321492; //y of id#198

  goals[20].x = 4.731087+ offset_x;   //x of id#1315 (outside north quad, east entrance)
  goals[20].y = -12.982945; //y of id#1315

  goals[21].x = -13.2928+ offset_x; //x of id#1714 (outside south quad, west entrance)
  goals[21].y = 5.05869;  //y of id#1714

  goals[22].x = -14.6241+ offset_x; //x of id#855 (Phil's office)
  goals[22].y = -1.31144; //y of id#855
  
  goals[23].x = 4.17692+ offset_x;  //x of id#1529 (Ventura's office)
  goals[23].y = -6.37767; //y of id#1529

  goals[24].x = 3.84982+ offset_x;  //x of id#1151 (outside north quad, west entrance)
  goals[24].y = 6.40079; //y of id#1151
  //---------end new building 14 goals------------
  
  for(int i=0;i<10;i++) { //max 10 routes
    for(int j=0;j<20;j++) { //max 20 waypoints per route
      routes[i].pause[j] = 0;
    }
  }

  //north quad clockwise loop 
  //heading (clockwise convention): 0=north, 90=east, 180=south, 270=west
  routes[0].waypoints[0] = 0; //office
  routes[0].heading[0] = 0;
  routes[0].waypoints[1] = 4; //Kurfess
  routes[0].heading[1] = 270;
  routes[0].waypoints[2] = 9; //outside north quad, west entrance
  routes[0].heading[2] = 0;
  routes[0].waypoints[3] = 1; //men's bathroom
  routes[0].heading[3] = 90;
  routes[0].waypoints[4] = 2; //women's bathroom
  routes[0].heading[4] = 180;
  routes[0].waypoints[5] = 5; //outside north quad, east entrance
  routes[0].heading[5] = 270;
  routes[0].waypoints[6] = 8; //Ventura
  routes[0].heading[6] = 180;
  routes[0].length = 7;

  //north quad counterclockwise loop
  routes[1].waypoints[0] = 0; //office
  routes[1].heading[0] = 0;
  routes[1].waypoints[1] = 8; //Ventura
  routes[1].heading[1] = 90;
  routes[1].waypoints[2] = 5; //north quad, east 
  routes[1].heading[2] = 0;
  routes[1].waypoints[3] = 2; //women's bathroom
  routes[1].heading[3] = 270;
  routes[1].waypoints[4] = 1; //men's bathroom
  routes[1].heading[4] = 180;
  routes[1].waypoints[5] = 9; //outside north quad, west entrance
  routes[1].heading[5] = 90;
  routes[1].waypoints[6] = 4; //Kurfess
  routes[1].heading[6] = 180;
  routes[1].length = 7;

  //south quad counterclockwise loop
  routes[2].waypoints[0] = 0; //office
  routes[2].heading[0] = 0;
  routes[2].waypoints[1] = 4; //Kurfess
  routes[2].heading[1] = 270;
  routes[2].waypoints[2] = 9; //north quad, outside west
  routes[2].heading[2] = 180;
  routes[2].waypoints[3] = 6; //south quad, outside west
  routes[2].heading[3] = 90;
  routes[2].waypoints[4] = 3; //south quad, outside east
  routes[2].heading[4] = 0;
  routes[2].waypoints[5] = 5; //outside north quad, east entrance
  routes[2].heading[5] = 270;
  routes[2].waypoints[6] = 8; //Ventura
  routes[2].heading[6] = 90;
  routes[2].length = 7;

  //south quad clockwise loop
  routes[3].waypoints[0] = 0; //office
  routes[3].heading[0] = 0;
  routes[3].waypoints[1] = 8; //Ventura
  routes[3].heading[1] = 90;
  routes[3].waypoints[2] = 5; //outside north quad, east entrance
  routes[3].heading[2] = 180;
  routes[3].waypoints[3] = 3; //outside south quad, east entrance
  routes[3].heading[3] = 270;
  routes[3].waypoints[4] = 6; //south quad, outside west
  routes[3].heading[4] = 0;
  routes[3].waypoints[5] = 9; //outside north quad, west entrance
  routes[3].heading[5] = 90;
  routes[3].waypoints[6] = 4; //Kurfess
  routes[3].heading[6] = 180;
  routes[3].length = 7;

  //north quad, west hallway loop
  routes[4].waypoints[0] = 0; //office
  routes[4].heading[0] = 0;
  routes[4].waypoints[1] = 4; //Kurfess
  routes[4].heading[1] = 270;
  routes[4].waypoints[2] = 9; //outside north quad, west entrance
  routes[4].heading[2] = 180;
  routes[4].waypoints[3] = 6; //south quad, outside west
  routes[4].heading[3] = 0;
  routes[4].waypoints[4] = 9; //outside north quad, west entrance
  routes[4].heading[4] = 90;
  routes[4].waypoints[5] = 4; //Kurfess
  routes[4].heading[5] = 180;
  routes[4].length = 6;

  //long outside loop
  routes[5].waypoints[0] = 0; //office
  routes[5].heading[0] = 0;
  routes[5].waypoints[1] = 4; //Kurfess
  routes[5].heading[1] = 270;
  routes[5].waypoints[2] = 9; //outside north quad, west entrance
  routes[5].heading[2] = 180;
  routes[5].waypoints[3] = 6; //south quad, outside west
  routes[5].heading[3] = 0;
  routes[5].waypoints[4] = 1; //men's bathroom
  routes[5].heading[4] = 90;
  routes[5].waypoints[5] = 2; //women's bathroom
  routes[5].heading[5] = 180;
  routes[5].waypoints[6] = 3; //outside south quad, east entrance
  routes[5].heading[6] = 0;
  routes[5].waypoints[7] = 5; //outside north quad, east entrance
  routes[5].heading[7] = 270;
  routes[5].waypoints[8] = 8; //Ventura
  routes[5].heading[8] = 180;
  routes[5].length = 9;

  //testing: north quad, east hallway loop
  routes[6].waypoints[0] = 0; //office
  routes[6].heading[0] = 0;
  routes[6].waypoints[1] = 8; //Ventura
  routes[6].heading[1] = 90;
  routes[6].waypoints[2] = 5; //outside north quad, east entrance
  routes[6].heading[2] = 0;
  routes[6].waypoints[3] = 2; //women's bathroom
  routes[6].heading[3] = 180;
  routes[6].waypoints[4] = 3; //outside south quad, east entrance
  routes[6].heading[4] = 0;
  routes[6].waypoints[5] = 5; //outside north quad, east entrance
  routes[6].heading[5] = 270;
  routes[6].waypoints[6] = 8; //Ventura
  routes[6].heading[6] = 180;
  routes[6].length = 7;

  //testing: Dexter lawn
  routes[7].waypoints[0] = 10; //waypoint sign
  routes[7].heading[0] = 0;
  routes[7].waypoints[1] = 11; //5m away from sign
  routes[7].heading[1] = 0;
  routes[7].waypoints[2] = 10; //waypoint sign
  routes[7].heading[2] = 0;
  routes[7].waypoints[3] = 12; //right of the sign
  routes[7].heading[3] = 0;
  routes[7].waypoints[4] = 13; //
  routes[7].heading[4] = 0;
  routes[7].waypoints[5] = 14; //cross diagonal
  routes[7].heading[5] = 0;
  routes[7].waypoints[6] = 13; //
  routes[7].heading[6] = 0;
  routes[7].waypoints[7] = 12; //right of the sign
  routes[7].heading[7] = 0;
  routes[7].length = 8;

  //new building 14 map - long outside loop
  routes[8].waypoints[0] = 15; //office
  routes[8].heading[0] = 0;
  routes[8].waypoints[1] = 19; //Kurfess
  routes[8].heading[1] = 270;
  routes[8].waypoints[2] = 24; //outside north quad, west entrance
  routes[8].heading[2] = 180;
  routes[8].waypoints[3] = 21; //south quad, outside west
  routes[8].heading[3] = 0;
  routes[8].waypoints[4] = 16; //men's bathroom
  routes[8].heading[4] = 90;
  routes[8].waypoints[5] = 17; //women's bathroom
  routes[8].heading[5] = 180;
  routes[8].waypoints[6] = 18; //outside south quad, east entrance
  routes[8].heading[6] = 0;
  routes[8].waypoints[7] = 20; //outside north quad, east entrance
  routes[8].heading[7] = 270;
  routes[8].waypoints[8] = 23; //Ventura
  routes[8].heading[8] = 180;
  routes[8].length = 9;

  current_route = 5;  //old building 14
  //current_route = 7; //Dexter lawn
  //current_route = 8;  //new building 14
}

void Navigation::route_thread() {
  int begin = 0;
  int loop_complete = 0;

  while(ros::ok()) {
    route_mutex.lock();
    if (start_route == 1) {
      begin = 1;
      start_route = 0;
      loop_complete = 0;
    }
    route_mutex.unlock();

    if (begin == 1) {
      //start sending goals
      int goal_index, goal_heading;
      cur_goal_in_route = 0;

      while (cur_goal_in_route < routes[current_route].length) {
        goal_index = routes[current_route].waypoints[cur_goal_in_route];
        current_goal_index = goal_index;
        goal_heading = routes[current_route].heading[cur_goal_in_route];
        move_base_msgs::MoveBaseGoal goal;

        fill_goal_info(&goal, goal_index, goal_heading);

        ROS_INFO("Sending goal");
        ac->sendGoal(goal);
        ac->waitForResult();

        if(ac->getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
          ROS_INFO("Hooray, reached goal");

          if (loop_complete == 1) { //if just completed a loop, then break
            loop_complete = 0;
            break;
          }

          cur_goal_in_route = (cur_goal_in_route + 1) % routes[current_route].length;

          if (cur_goal_in_route == 0) {
            loop_complete = 1;  //just completed a loop through the waypoints
          }
        } else {
          ROS_INFO("The base failed to reach goal");
        }
      }
    }

    begin = 0;
    
    /* ROS_INFO("test"); */
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
  }
}

void Navigation::enable_rtabmap_mapping(int enable) {
    dynamic_reconfigure::ReconfigureRequest srv_req;
    dynamic_reconfigure::ReconfigureResponse srv_resp;
    dynamic_reconfigure::DoubleParameter double_param;
    dynamic_reconfigure::Config conf;

    double_param.name = "sim_time";
    double_param.value = 2.0;
    conf.doubles.push_back(double_param);

    srv_req.config = conf;

    if (enable == 1) {
      ros::service::call("/rtabmap/set_mode/localization", srv_req, srv_resp);
    } else {
      ros::service::call("/rtabmap/set_mode/mapping", srv_req, srv_resp);
    }
}

void Navigation::check_distance_thread() {
  while(ros::ok()) {
    if (get_distance_from_goal() > 2.0) {
      dynamic_reconfigure::ReconfigureRequest srv_req;
      dynamic_reconfigure::ReconfigureResponse srv_resp;
      dynamic_reconfigure::DoubleParameter double_param;
      dynamic_reconfigure::Config conf;

      double_param.name = "sim_time";
      double_param.value = 2.0;
      conf.doubles.push_back(double_param);

      srv_req.config = conf;

      ros::service::call("/planner/move_base/TrajectoryPlannerROS/set_parameters", srv_req, srv_resp);
    } else {
      dynamic_reconfigure::ReconfigureRequest srv_req;
      dynamic_reconfigure::ReconfigureResponse srv_resp;
      dynamic_reconfigure::DoubleParameter double_param;
      dynamic_reconfigure::Config conf;

      double_param.name = "sim_time";
      double_param.value = 1.2;
      conf.doubles.push_back(double_param);

      srv_req.config = conf;

      ros::service::call("/planner/move_base/TrajectoryPlannerROS/set_parameters", srv_req, srv_resp);
    }

    /* double s_time; */
    /* nh->getParam("/planner/move_base/TrajectoryPlannerROS/sim_time", s_time); */
    /* ROS_INFO("sim_time: %f", s_time); */
    /* ROS_INFO("distance from goal: %f", get_distance_from_goal()); */
    boost::this_thread::sleep_for(boost::chrono::milliseconds(5000));
  }
}

void Navigation::planner_cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist) {
  planner_cmd_mutex.lock();
  linear_x = twist->linear.x;
  angular_z = -twist->angular.z;
  planner_cmd_mutex.unlock();

  return;
}

//thread to check if the robot has not moved and the goal is still active
//if that is true, then something is blocking the path, so play a sound
void Navigation::check_movement_thread() {
  int stop_counter = 0;

  boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));

  while(ros::ok()) {
    float linear;
    float angular;

    planner_cmd_mutex.lock();
    linear = linear_x;
    angular = angular_z;
    planner_cmd_mutex.unlock();

    if ((linear < .01) && (abs(angular) < .01) && (ac->getState() == actionlib::SimpleClientGoalState::ACTIVE)) {
    /* if ((linear < .01) && (angular < .01) ) { */
      stop_counter++; 
    } else {
      stop_counter = 0;
    }

    if (stop_counter == 2) {
      //play sound
      int sound_count = 1; //TODO: convert this to a random number
      std::string str = "play /home/jseng/" + std::to_string(sound_count) + ".wav";
      char* cstr = new char[50];
      std::strcpy(cstr, str.c_str());

      int i;
      i = system (cstr);
      //ROS_INFO("test");
      i = 0;
      stop_counter = i;
      delete[] cstr;
    }

    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
  }
}

void Navigation::set_heading(int degrees, double* w, double* x, double* y, double* z) {
  if (degrees == 0) {
    *w = 1;
    *x = 0;
    *y = 0;
    *z = 0;
  } else if (degrees == 180) {
    *w = 0;
    *x = 0;
    *y = 0;
    *z = 1;
  } else if (degrees == 90) {
    *w = 0.7071;
    *x = 0;
    *y = 0;
    *z = -0.7071;
  } else { //heading = 270
    *w = 0.7071;
    *x = 0;
    *y = 0;
    *z = 0.7071;
  }
}

void Navigation::init_action_client() {
  //wait for the action server to come up
  while(!ac->waitForServer(ros::Duration(5.0))){
    ROS_INFO("Waiting for the move_base action server to come up");
  }
}

void Navigation::set_action_client(MoveBaseClient* a) {
  ac = a;
}

//fill in the goal with pose and heading
void Navigation::fill_goal_info(move_base_msgs::MoveBaseGoal *goal, int goal_index, int heading) {
  goal->target_pose.header.frame_id = "map";
  goal->target_pose.header.stamp = ros::Time::now();

  goal->target_pose.pose.position.x = goals[goal_index].x;
  goal->target_pose.pose.position.y = goals[goal_index].y;
  goal->target_pose.pose.position.z = 0;

  set_heading(heading,
      &(goal->target_pose.pose.orientation.w),
      &(goal->target_pose.pose.orientation.x),
      &(goal->target_pose.pose.orientation.y),
      &(goal->target_pose.pose.orientation.z)
      );
}

//return the distance from the current position to the goal
double Navigation::get_distance_from_goal() {
  geometry_msgs::TransformStamped transformStamped;

  try{
    transformStamped = tfBuffer_.lookupTransform("map", "base_link", ros::Time(0));
  }
  catch (tf2::TransformException &ex) {
    ROS_WARN("%s",ex.what());
    return -1;  //if error, return -1
  }

  double x_disp = transformStamped.transform.translation.x;
  double y_disp = transformStamped.transform.translation.y;

  double x_dest = goals[current_goal_index].x;
  double y_dest = goals[current_goal_index].y;

  ROS_INFO("xdisp: %f xdest: %f y_disp: %f y_dest: %f", x_disp, x_dest, y_disp, y_dest);
  double dist = pow((pow(x_dest - x_disp, 2) + pow(y_dest - y_disp, 2)), .5);
  return dist;
}

void Navigation::send_goal_callback(const std_msgs::Int8::ConstPtr& mesg) {
  route_mutex.lock();
  start_route = 1;
  route_mutex.unlock();
  return;
}

int main(int argc, char** argv){
  ros::init(argc, argv, "navigation_goals");
  ros::NodeHandle n;
  nh = &n;
  tfListener_ = new tf2_ros::TransformListener(tfBuffer_);

  Navigation nav;
  MoveBaseClient a("planner/move_base", true);
  nav.set_action_client(&a);
  nav.init_action_client();

  ros::spin();
  ros::waitForShutdown();

  return 0;
}
