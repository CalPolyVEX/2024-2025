#include <ros/ros.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Int8.h>
#include <actionlib/client/simple_action_client.h>
#include <iostream>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/TransformStamped.h>
#include <cmath>

using namespace std;

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;
ros::NodeHandle* nh;

struct goal {
  double x;
  double y;
  int id;
  int heading;
};

struct route {
  int waypoints[20];
  int heading[20];
  int length;
};

class Navigation {
  ros::Subscriber goal_sub;
  MoveBaseClient* ac;
  struct goal goals[40]; //database of all the goals
  struct route routes[10];
  int cur_goal_in_route = 0; //the current goal index in the route
  int current_route;
  int current_goal_index;

  public:
    Navigation();
    void init_action_client();
    void send_goal_callback(const std_msgs::Int8::ConstPtr& mesg);
    void set_action_client(MoveBaseClient* a);
    void run_loop();
    void set_heading(int degrees, double* w, double* x, double* y, double* z);
    double get_distance_from_goal();
    void fill_goal_info(move_base_msgs::MoveBaseGoal *goal, int goal_index, int heading);
};

Navigation::Navigation() {
  //the /autonomous topic send a message if a goal is requested
  goal_sub = nh->subscribe("/autonomous",1,&Navigation::send_goal_callback,this); 

  goals[0].x = .5;       //x of id#1 (Seng office)
  goals[0].y = 0;        //y of id#1
  goals[0].id = 1;       

  goals[1].x = 14.5542;  //x of id#65 (men's bathroom)
  goals[1].y = 6.03422;  //y of id#65
  goals[1].id = 65;  

  goals[2].x = 15.2227;  //x of id#369 (women's bathroom)
  goals[2].y = -11.9279; //y of id#369
  goals[2].id = 369;  

  goals[3].x = -12.203691; //x of id#1315 (outside south quad, east entrance)
  goals[3].y = -13.27466; //y of id#1315
  goals[3].id = 1315;  

  goals[4].x = 3.96606;   //x of id#198 (Kurfess office)
  goals[4].y = .321492; //y of id#198
  goals[4].id = 198;  

  goals[5].x = 4.531087;   //x of id#1315 (outside north quad, east entrance)
  goals[5].y = -12.982945; //y of id#1315
  goals[5].id = 1041;  

  goals[6].x = -13.6928; //x of id#1714 (outside south quad, west entrance)
  goals[6].y = 5.05869;  //y of id#1714
  goals[6].id = 1714;  

  goals[7].x = -14.6241; //x of id#855 (Phil's office)
  goals[7].y = -1.31144; //y of id#855
  goals[7].id = 855;  
  
  goals[8].x = 4.17692;  //x of id#1529 (Ventura's office)
  goals[8].y = -6.37767; //y of id#1529
  goals[8].id = 1529;  

  goals[9].x = 3.84982;  //x of id#1151 (outside north quad, west entrance)
  goals[9].y = 6.40079; //y of id#1151
  goals[9].id = 1151;  

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
  routes[1].length = 6;

  //south quad counterclockwise loop
  routes[2].waypoints[0] = 0; //office
  routes[2].heading[0] = 0;
  routes[2].waypoints[1] = 4; //Kurfess
  routes[2].heading[1] = 270;
  routes[2].waypoints[2] = 9; //north quad, outside west
  routes[2].heading[2] = 180;
  routes[2].waypoints[3] = 6; //south quad, outside wes
  routes[2].heading[3] = 90;
  routes[2].waypoints[4] = 9; //north quad, outside west
  routes[2].heading[4] = 90;
  routes[2].length = 5;

  //testing: south quad clockwise loop
  routes[3].waypoints[0] = 0; //office
  routes[3].heading[0] = 0;
  routes[3].waypoints[1] = 8; //Ventura
  routes[3].heading[1] = 90;
  routes[3].waypoints[2] = 5; //outside north quad, east entrance
  routes[3].heading[2] = 180;
  routes[3].waypoints[3] = 3; //outside south quad, east entrance
  routes[3].heading[3] = 270;
  routes[3].waypoints[4] = 5; //outside north quad, east entrance
  routes[3].heading[4] = 270;
  routes[3].waypoints[5] = 8; //Ventura
  routes[3].heading[5] = 180;
  routes[3].length = 6;

  //testing:  north quad clockwise loop
  routes[4].waypoints[0] = 0; //office
  routes[4].heading[0] = 0;
  routes[4].waypoints[1] = 4; //Kurfess
  routes[4].heading[1] = 270;
  routes[4].waypoints[2] = 9; //outside north quad, west entrance
  routes[4].heading[2] = 90;
  routes[4].waypoints[3] = 4; //Kurfess
  routes[4].heading[3] = 180;
  routes[4].length = 4;

  current_route = 0;
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

void Navigation::run_loop() {
  int goal_index, goal_heading;
  tf2_ros::Buffer tfBuffer;
  tf2_ros::TransformListener tfListener(tfBuffer);

  ros::Rate rate(10.0);

  //send first goal
  cur_goal_in_route = 0;
  goal_index = routes[current_route].waypoints[0];
  goal_heading = routes[current_route].heading[0];
  move_base_msgs::MoveBaseGoal goal;

  fill_goal_info(&goal, goal_index, goal_heading);

  ROS_INFO("Sending goal");
  ac->sendGoal(goal);

  while (nh->ok()){
    double dist = get_distance_from_goal();

    if (dist < 3.0) {
      cur_goal_in_route = (cur_goal_in_route + 1) % 4;

      //send goal
      goal_index = routes[current_route].waypoints[cur_goal_in_route];
      goal_heading = routes[current_route].heading[cur_goal_in_route];
      move_base_msgs::MoveBaseGoal goal;

      fill_goal_info(&goal, goal_index, goal_heading);

      ROS_INFO("Sending goal");
      ac->sendGoal(goal);
    }

    rate.sleep();
  }
}

//return the distance from the current position to the goal
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
  tf2_ros::Buffer tfBuffer;
  tf2_ros::TransformListener tfListener(tfBuffer);

  geometry_msgs::TransformStamped transformStamped;
  try{
    transformStamped = tfBuffer.lookupTransform("map", "base_link", ros::Time(0));
  }
  catch (tf2::TransformException &ex) {
    ROS_WARN("%s",ex.what());
    return -1;  //if error, return -1
  }

  double x_disp = transformStamped.transform.translation.x;
  double y_disp = transformStamped.transform.translation.y;

  double x_dest = goals[current_goal_index].x;
  double y_dest = goals[current_goal_index].y;

  double dist = pow((pow(x_dest - x_disp, 2) + pow(y_dest - y_disp, 2)), .5);
  return dist;
}

void Navigation::send_goal_callback(const std_msgs::Int8::ConstPtr& mesg) {
  if (mesg->data == 1) { //received new navigation goal
    move_base_msgs::MoveBaseGoal goal;

    goal.target_pose.header.frame_id = "map";
    goal.target_pose.header.stamp = ros::Time::now();

    current_goal_index = routes[current_route].waypoints[cur_goal_in_route];
    goal.target_pose.pose.position.x = goals[current_goal_index].x;
    goal.target_pose.pose.position.y = goals[current_goal_index].y;
    goal.target_pose.pose.position.z = 0;

    set_heading(routes[current_route].heading[cur_goal_in_route],
      &(goal.target_pose.pose.orientation.w),
      &(goal.target_pose.pose.orientation.x),
      &(goal.target_pose.pose.orientation.y),
      &(goal.target_pose.pose.orientation.z)
      );

    ROS_INFO("Sending goal");
    ac->sendGoal(goal);

    ac->waitForResult();

    if(ac->getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
      ROS_INFO("Hooray, reached goal");
      cur_goal_in_route = (cur_goal_in_route + 1) % routes[current_route].length;
    } else {
      ROS_INFO("The base failed to reach goal");
    }
  } else if (mesg->data == 2) {
    //cancel all goals
    ac->cancelAllGoals();
  } else if (mesg->data == 3) { //testing
  }
}

int main(int argc, char** argv){
  ros::init(argc, argv, "navigation_goals");
  ros::NodeHandle n;
  nh = &n;

  Navigation nav;
  MoveBaseClient a("planner/move_base", true);
  nav.set_action_client(&a);
  nav.init_action_client();

  ros::spin();
  ros::waitForShutdown();

  return 0;
}
