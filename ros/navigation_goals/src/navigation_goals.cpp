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

class Navigation {
  ros::Subscriber goal_sub;
  MoveBaseClient* ac;
  double goals[4][2];
  int goal_counter = 3;

  public:
    Navigation();
    void init_action_client();
    void send_goal_callback(const std_msgs::Int8::ConstPtr& mesg);
    void set_action_client(MoveBaseClient* a);
    void run_loop();
};

Navigation::Navigation() {
  //tell the action client that we want to spin a thread by default
  /* MoveBaseClient ac("move_base", true); */
  //ac = MoveBaseClient("planner/move_base", true);

  goal_sub = nh->subscribe("/autonomous",1,&Navigation::send_goal_callback,this); 

  goals[0][0] = 14.0542;  //x of id#65 (men's bathroom)
  goals[0][1] = 6.03422;  //y of id#65

  goals[1][0] = 15.2227;  //x of id#369 (women's bathroom)
  goals[1][1] = -11.4279; //y of id#369

  goals[2][0] = -12.9478;        //x of id#1315 (south quad)
  goals[2][1] = -12.7342;        //y of id#1315

  goals[3][0] = .5;        //x of id#1 (office)
  goals[3][1] = 0;        //y of id#1
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

  goal.target_pose.pose.position.x = goals[goal_counter][0];
  goal.target_pose.pose.position.y = goals[goal_counter][1];
  goal.target_pose.pose.position.z = 0;
  goal.target_pose.pose.orientation.w = 1.0;
  goal.target_pose.pose.orientation.x = 0;
  goal.target_pose.pose.orientation.y = 0;
  goal.target_pose.pose.orientation.z = 0;

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

    double x_dest = goals[goal_counter][0];
    double y_dest = goals[goal_counter][1];

    double dist = pow((pow(x_dest - x_disp, 2) + pow(y_dest - y_disp, 2)), .5);

    if (dist < 3.0) {
      goal_counter = (goal_counter + 1) % 4;

      //send goal
      move_base_msgs::MoveBaseGoal goal;

      goal.target_pose.header.frame_id = "map";
      goal.target_pose.header.stamp = ros::Time::now();

      goal.target_pose.pose.position.x = goals[goal_counter][0];
      goal.target_pose.pose.position.y = goals[goal_counter][1];
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

    goal.target_pose.pose.position.x = goals[goal_counter][0];
    goal.target_pose.pose.position.y = goals[goal_counter][1];
    goal.target_pose.pose.position.z = 0;
    goal.target_pose.pose.orientation.w = 1.0;
    goal.target_pose.pose.orientation.x = 0;
    goal.target_pose.pose.orientation.y = 0;
    goal.target_pose.pose.orientation.z = 0;

    ROS_INFO("Sending goal");
    ac->sendGoal(goal);

    ac->waitForResult();

    if(ac->getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {
      ROS_INFO("Hooray, the base moved 1 meter forward");
      goal_counter = (goal_counter + 1) % 4;
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
