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
  struct route routes[4];
  int goal_counter = 0;

  public:
    Navigation();
    void init_action_client();
    void send_goal_callback(const std_msgs::Int8::ConstPtr& mesg);
    void set_action_client(MoveBaseClient* a);
    void run_loop();
    void set_heading(int degrees, double* w, double* x, double* y, double* z);
};

Navigation::Navigation() {
  //tell the action client that we want to spin a thread by default
  /* MoveBaseClient ac("move_base", true); */
  //ac = MoveBaseClient("planner/move_base", true);

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

  goals[3].x = -12.9478; //x of id#1315 (outside south quad, east entrance)
  goals[3].y = -12.7342; //y of id#1315
  goals[3].id = 1315;  

  goals[4].x = 3.21606;   //x of id#198 (Kurfess office)
  goals[4].y = -0.235708; //y of id#198
  goals[4].id = 198;  

  goals[5].x = 3.8924;   //x of id#1315 (outside north quad, east entrance)
  goals[5].y = -13.6663; //y of id#1315
  goals[5].id = 1315;  

  goals[6].x = -13.8928; //x of id#1714 (outside south quad, west entrance)
  goals[6].y = 4.95869;  //y of id#1714
  goals[6].id = 1714;  

  goals[7].x = -14.6241; //x of id#855 (Phil's office)
  goals[7].y = -1.31144; //y of id#855
  goals[7].id = 855;  
  
  goals[8].x = 3.97692;  //x of id#1529 (Ventura's office)
  goals[8].y = -6.97767; //y of id#1529
  goals[8].id = 1529;  

  goals[9].x = 4.30702;  //x of id#1151 (outside north quad, west entrance)
  goals[9].y = 5.90079; //y of id#1151
  goals[9].id = 1151;  

  routes[0].waypoints[0] = 0; //office
  routes[0].heading[0] = 0;
  routes[0].waypoints[1] = 4; //Kurfess
  routes[0].heading[1] = 270;
  routes[0].waypoints[2] = 1; //men's bathroom
  routes[0].heading[2] = 90;
  routes[0].waypoints[3] = 2; //women's bathroom
  routes[0].heading[3] = 180;
  routes[0].waypoints[4] = 5; //outside north quad, east entrance
  routes[0].heading[4] = 270;
  routes[0].length = 5;

  /* routes[0].waypoints[0] = 0; //office */
  /* routes[0].heading[0] = 0; */
  /* routes[0].waypoints[1] = 1; //men's bathroom */
  /* routes[0].heading[1] = 90; */
  /* routes[0].waypoints[2] = 2; //women's bathroom */
  /* routes[0].heading[2] = 180; */
  /* routes[0].length = 3; */
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
  tf2_ros::Buffer tfBuffer;
  tf2_ros::TransformListener tfListener(tfBuffer);

  ros::Rate rate(10.0);

  //send first goal
  goal_counter = 0;
  move_base_msgs::MoveBaseGoal goal;

  goal.target_pose.header.frame_id = "map";
  goal.target_pose.header.stamp = ros::Time::now();

  goal.target_pose.pose.position.x = goals[routes[0].waypoints[goal_counter]].x;
  goal.target_pose.pose.position.y = goals[routes[0].waypoints[goal_counter]].y;
  goal.target_pose.pose.position.z = 0;

  set_heading(routes[0].heading[goal_counter],
    &(goal.target_pose.pose.orientation.w),
    &(goal.target_pose.pose.orientation.x),
    &(goal.target_pose.pose.orientation.y),
    &(goal.target_pose.pose.orientation.z)
    );

  ROS_INFO("Sending goal");
  ac->sendGoal(goal);

  while (nh->ok()){
    geometry_msgs::TransformStamped transformStamped;
    try{
      transformStamped = tfBuffer.lookupTransform("map", "base_link", ros::Time(0));
    }
    catch (tf2::TransformException &ex) {
      ROS_WARN("%s",ex.what());
      ros::Duration(1.0).sleep();
      continue;
    }

    double x_disp = transformStamped.transform.translation.x;
    double y_disp = transformStamped.transform.translation.y;

    double x_dest = goals[goal_counter].x;
    double y_dest = goals[goal_counter].y;

    double dist = pow((pow(x_dest - x_disp, 2) + pow(y_dest - y_disp, 2)), .5);

    if (dist < 3.0) {
      goal_counter = (goal_counter + 1) % 4;

      //send goal
      move_base_msgs::MoveBaseGoal goal;

      goal.target_pose.header.frame_id = "map";
      goal.target_pose.header.stamp = ros::Time::now();

      goal.target_pose.pose.position.x = goals[goal_counter].x;
      goal.target_pose.pose.position.y = goals[goal_counter].y;
      goal.target_pose.pose.position.z = 0;
      goal.target_pose.pose.orientation.w = 1.0;
      goal.target_pose.pose.orientation.x = 0;
      goal.target_pose.pose.orientation.y = 0;
      goal.target_pose.pose.orientation.z = 0;

      ROS_INFO("Sending goal");
      ac->sendGoal(goal);
    }

    rate.sleep();
  }
}

void Navigation::send_goal_callback(const std_msgs::Int8::ConstPtr& mesg) {
  if (mesg->data == 1) { //received new navigation goal
    move_base_msgs::MoveBaseGoal goal;

    goal.target_pose.header.frame_id = "map";
    goal.target_pose.header.stamp = ros::Time::now();

    goal.target_pose.pose.position.x = goals[routes[0].waypoints[goal_counter]].x;
    goal.target_pose.pose.position.y = goals[routes[0].waypoints[goal_counter]].y;
    goal.target_pose.pose.position.z = 0;

    set_heading(routes[0].heading[goal_counter],
      &(goal.target_pose.pose.orientation.w),
      &(goal.target_pose.pose.orientation.x),
      &(goal.target_pose.pose.orientation.y),
      &(goal.target_pose.pose.orientation.z)
      );

    ROS_INFO("Sending goal");
    ac->sendGoal(goal);

    ac->waitForResult();

    if(ac->getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
      ROS_INFO("Hooray, the base moved 1 meter forward");
      goal_counter = (goal_counter + 1) % routes[0].length;
    } else {
      ROS_INFO("The base failed to move forward 1 meter for some reason");
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
