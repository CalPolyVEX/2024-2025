#include <ros/ros.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Int8.h>
#include <actionlib/client/simple_action_client.h>
#include <iostream>

using namespace std;

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;
ros::NodeHandle* nh;

class Navigation {
  ros::Subscriber goal_sub;
  MoveBaseClient* ac;

  public:
    Navigation();
    void init_action_client();
    void send_goal_callback(const std_msgs::Int8::ConstPtr& mesg);
    void set_action_client(MoveBaseClient* a);
};

Navigation::Navigation() {
  //tell the action client that we want to spin a thread by default
  /* MoveBaseClient ac("move_base", true); */
  //ac = MoveBaseClient("planner/move_base", true);

  goal_sub = nh->subscribe("/autonomous",1,&Navigation::send_goal_callback,this); 
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

void Navigation::send_goal_callback(const std_msgs::Int8::ConstPtr& mesg) {
  if (mesg->data == 1) { //received new navigation goal
    move_base_msgs::MoveBaseGoal goal;

    //we'll send a goal to the robot to move 1 meter forward
    goal.target_pose.header.frame_id = "map";
    goal.target_pose.header.stamp = ros::Time::now();

    goal.target_pose.pose.position.x = .5;
    goal.target_pose.pose.orientation.w = 1.0;

    ROS_INFO("Sending goal");
    ac->sendGoal(goal);

    ac->waitForResult();

    if(ac->getState() == actionlib::SimpleClientGoalState::SUCCEEDED)
      ROS_INFO("Hooray, the base moved 1 meter forward");
    else
      ROS_INFO("The base failed to move forward 1 meter for some reason");
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
