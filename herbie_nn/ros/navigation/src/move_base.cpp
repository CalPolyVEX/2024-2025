//6/16/19 This file contains functions for the graph map

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <geometry_msgs/Twist.h>
#include <ros/console.h>
#include <iostream>
#include <boost/thread.hpp>
#include <cmath>
#include "navigation.h"

#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>
#include <pcl_conversions/pcl_conversions.h>
#include <sensor_msgs/PointCloud2.h>

extern int sim_mode;

using namespace std;
typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

void Navigation::send_goal() { 
   //tell the action client that we want to spin a thread by default
   //MoveBaseClient ac("move_base", true);

   //wait for the action server to come up
   while(!ac->waitForServer(ros::Duration(5.0))){
      ROS_INFO("Waiting for the move_base action server to come up");
   }

   move_base_msgs::MoveBaseGoal goal;

   //we'll send a goal to the robot to move 1 meter forward
   goal.target_pose.header.frame_id = "base_link";
   goal.target_pose.header.stamp = ros::Time::now();

   goal.target_pose.pose.position.x = 1.0;
   goal.target_pose.pose.orientation.w = 1.0;

   ROS_INFO("Sending goal");
   ac->sendGoal(goal);

   ac->waitForResult();

   if(ac->getState() == actionlib::SimpleClientGoalState::SUCCEEDED)
      ROS_INFO("Hooray, the base moved 1 meter forward");
   else
      ROS_INFO("The base failed to move forward 1 meter for some reason");
}

void Navigation::publish_pointcloud() {
   int numpoints = 80; //the number of points output by the neural network
   sensor_msgs::PointCloud2 cloud_msg;

   cloud_msg.header.frame_id = "see3_cam";
   cloud_msg.header.stamp = ros::Time::now();
   cloud_msg.width  = (numpoints-1)*4; //number of points (interpolate 4 times the points)
   cloud_msg.height = 1;
   cloud_msg.is_bigendian = false;
   cloud_msg.is_dense = true; // there may be invalid points

   sensor_msgs::PointCloud2Modifier modifier(cloud_msg);
   modifier.setPointCloud2FieldsByString(2,"xyz","rgb");
   modifier.resize((numpoints-1)*4); //interpolate 4 times the points

   //iterators
   sensor_msgs::PointCloud2Iterator<float> out_x(cloud_msg, "x");
   sensor_msgs::PointCloud2Iterator<float> out_y(cloud_msg, "y");
   sensor_msgs::PointCloud2Iterator<float> out_z(cloud_msg, "z");
   sensor_msgs::PointCloud2Iterator<uint8_t> out_r(cloud_msg, "r");
   sensor_msgs::PointCloud2Iterator<uint8_t> out_g(cloud_msg, "g");
   sensor_msgs::PointCloud2Iterator<uint8_t> out_b(cloud_msg, "b");

   double r_t_inv[3][3] = {{-0.00297077239028572,-4.4139112416322384e-05,0.9313145025868853},
             {2.3482445189653307e-05,-0.0024911437766359894,1.1922101009254173},
             {-5.526553689847709e-05,0.004262116239831571,-0.19803144040722234}};

   for (int i=0; i<(numpoints-1); i++)
   {
      double x = i*8;
      double y = ground[i] * 360;

      double next_x = (i+1)*8;
      double next_y = ground[i+1] * 360;

      /*          imagepoint = np.array([x,y,1],dtype=np.float64) */
      /*          ans = np.matmul(r_t_inv, imagepoint) */

      /*          w = ans.item(2) */
      /*          ans = ans / w */
      /*          ans = ans.tolist()[0:2] #answer is in meters */
      //do matrix multiply
      //0,0 0,1 0,2   x
      //1,0 1,1 1,2   y
      //2,0 2,1 2,2   1
      double a = (r_t_inv[0][0] * x) + (r_t_inv[0][1] * y) + (r_t_inv[0][2] * 1); 
      double b = (r_t_inv[1][0] * x) + (r_t_inv[1][1] * y) + (r_t_inv[1][2] * 1); 
      double c = (r_t_inv[2][0] * x) + (r_t_inv[2][1] * y) + (r_t_inv[2][2] * 1); 

      double next_a = (r_t_inv[0][0] * next_x) + (r_t_inv[0][1] * next_y) + (r_t_inv[0][2] * 1); 
      double next_b = (r_t_inv[1][0] * next_x) + (r_t_inv[1][1] * next_y) + (r_t_inv[1][2] * 1); 
      double next_c = (r_t_inv[2][0] * next_x) + (r_t_inv[2][1] * next_y) + (r_t_inv[2][2] * 1); 

      a = a / c;
      b = b / c;
      next_a = next_a / next_c;
      next_b = next_b / next_c;

      *out_x = b; //positive x-axis is forward from robot
      *out_y = a; //positive y-axis is to the left of robot
      *out_z = 0.2; //positive z-axis is up from the robot

      // store colors
      *out_r = 255;
      *out_g = 255;
      *out_b = 255;

      //increment
      ++out_x;
      ++out_y;
      ++out_z;
      ++out_r;
      ++out_g;
      ++out_b;

      *out_x = b + ((next_b - b) * .25); //positive x-axis is forward from robot
      *out_y = a + ((next_a - a) * .25); //positive y-axis is to the left of robot
      *out_z = 0.2; //positive z-axis is up from the robot

      // store colors
      *out_r = 255;
      *out_g = 255;
      *out_b = 255;

      //increment
      ++out_x;
      ++out_y;
      ++out_z;
      ++out_r;
      ++out_g;
      ++out_b;

      *out_x = b + ((next_b - b) * .5); //positive x-axis is forward from robot
      *out_y = a + ((next_a - a) * .5); //positive y-axis is to the left of robot
      *out_z = 0.2; //positive z-axis is up from the robot

      // store colors
      *out_r = 255;
      *out_g = 255;
      *out_b = 255;

      //increment
      ++out_x;
      ++out_y;
      ++out_z;
      ++out_r;
      ++out_g;
      ++out_b;

      *out_x = b + ((next_b - b) * .75); //positive x-axis is forward from robot
      *out_y = a + ((next_a - a) * .75); //positive y-axis is to the left of robot
      *out_z = 0.2; //positive z-axis is up from the robot

      // store colors
      *out_r = 255;
      *out_g = 255;
      *out_b = 255;

      //increment
      ++out_x;
      ++out_y;
      ++out_z;
      ++out_r;
      ++out_g;
      ++out_b;
   }

   pointcloud_pub_.publish(cloud_msg);

   /* new_point_list = [] */
   /*    for i in range(len(self.point_list)-1): */
   /*       diff_x = (self.point_list[i+1][0] - self.point_list[i][0]) */
   /*       diff_y = (self.point_list[i+1][1] - self.point_list[i][1]) */

   /*       new_x = self.point_list[i][0] + .333 * diff_x */
   /*          new_y = self.point_list[i][1] + .333 * diff_y */

   /*          new_x2 = self.point_list[i][0] + .667 * diff_x */
   /*          new_y2 = self.point_list[i][1] + .667 * diff_y */

   /*          new_point_list.append(self.point_list[i]) */
   /*          new_point_list.append((new_x,new_y)) */
   /*          new_point_list.append((new_x2,new_y2)) */

   /*      cloud_points = [] */
   /*      #for p in self.point_list: */
   /*      for p in new_point_list: */
   /*          x = p[0] */
   /*          y = p[1] */
   /*          imagepoint = np.array([x,y,1],dtype=np.float64) */
   /*          ans = np.matmul(r_t_inv, imagepoint) */

   /*          w = ans.item(2) */
   /*          ans = ans / w */
   /*          ans = ans.tolist()[0:2] #answer is in meters */
   /*          cloud_points.append([ans[1], ans[0], 0]) */

        //#cloud_points = [[1.0, 1.0, 0.0],[1.0, 2.0, 0.0],[2,0,0]]

        //header
        /* header = std_msgs.msg.Header() */
        /* header.stamp = rospy.Time.now() */
        /* header.frame_id = 'see3_cam' */
        //create pcl from points
        /* scaled_polygon_pcl = pcl2.create_cloud_xyz32(header, cloud_points) */

}
