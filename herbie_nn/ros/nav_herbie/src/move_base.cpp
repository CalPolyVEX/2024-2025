#include "navigation.h"

#include <dynamic_reconfigure/DoubleParameter.h>
#include <dynamic_reconfigure/Reconfigure.h>
#include <dynamic_reconfigure/Config.h>

#include <pcl_conversions/pcl_conversions.h>
#include <sensor_msgs/PointCloud2.h>

extern int sim_mode;
extern ros::Timer diag_timer;

using namespace std;
typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

int Navigation::call_make_plan(double goal_x, double goal_y, double* pose_x, double* pose_y) {
   ros::ServiceClient serviceClient = nh.serviceClient<nav_msgs::GetPlan>(service_name);
   nav_msgs::GetPlan srv;

   //get the transform of the base_link
   geometry_msgs::TransformStamped transformStamped;
   transformStamped = tfBuffer.lookupTransform("odom", "base_link", ros::Time(0), ros::Duration(.5));

   //Request service: plan route
   srv.request.start.header.frame_id ="odom";
   srv.request.start.pose.position.x = transformStamped.transform.translation.x;
   srv.request.start.pose.position.y = transformStamped.transform.translation.y;
   srv.request.start.pose.position.z = 0;
   srv.request.start.pose.orientation.w = transformStamped.transform.rotation.w;
   srv.request.start.pose.orientation.x = transformStamped.transform.rotation.x;
   srv.request.start.pose.orientation.y = transformStamped.transform.rotation.y;
   srv.request.start.pose.orientation.z = transformStamped.transform.rotation.z;

   srv.request.goal.header.frame_id = "odom";
   srv.request.goal.pose.position.x = goal_x;//End point coordinates
   srv.request.goal.pose.position.y = goal_y;
   srv.request.goal.pose.orientation.w = 1.0;
   srv.request.tolerance = 0.5;//If the goal cannot be reached, the most recent constraint

   ROS_INFO("connect to %s",serviceClient.getService().c_str());
   
   //Perform the actual path planner call
   if (serviceClient.call(srv)) {
      //srv.response.plan.poses is the container to save the result, traverse and take out
      if (! srv.response.plan.poses.empty()) {
         ROS_INFO("Found a plan");
         int index = srv.response.plan.poses.size() - 1;
         geometry_msgs::PoseStamped p;
         p = srv.response.plan.poses.at(index);

         //set the actual final pose of the plan
         *pose_x = p.pose.position.x;
         *pose_y = p.pose.position.y; 

         /* boost::forEach(const geometry_msgs::PoseStamped &p, srv.response.plan.poses) { */
         /*    ROS_INFO("x = %f, y = %f", p.pose.position.x, p.pose.position.y); */
         /* } */
         return 1; //found a plan
      }
      else {
         ROS_WARN("Got empty plan");
         return 0;
      }
   }
   else {
      ROS_ERROR("Failed to call service %s - is the robot moving?",
            serviceClient.getService().c_str());
      return -1;
   }
}

void convert_image_to_world(int col, int row, double* x, double* y) {
   /*          imagepoint = np.array([x,y,1],dtype=np.float64) */
   /*          ans = np.matmul(r_t_inv, imagepoint) */

   /*          w = ans.item(2) */
   /*          ans = ans / w */
   /*          ans = ans.tolist()[0:2] #answer is in meters */
   //do matrix multiply
   //0,0 0,1 0,2   x
   //1,0 1,1 1,2   y
   //2,0 2,1 2,2   1
   //
   //9/5/21 calibration
   /* double r_t_inv[3][3] = {{-0.00297077239028572,-4.4139112416322384e-05,0.9313145025868853}, */
   /*           {2.3482445189653307e-05,-0.0024911437766359894,1.1922101009254173}, */
   /*           {-5.526553689847709e-05,0.004262116239831571,-0.19803144040722234}}; */

   //9/12/21 calibration
   double r_t_inv[3][3] = {{-0.00266937920376545,-0.00011232742977799345,0.8538495118346788},
      {3.7348555556903726e-05,-0.002714164927900428,1.1541406814343038},
      {-2.1784034424370855e-05,0.0033593149732969742,-0.10899350363712738}};


   double a = (r_t_inv[0][0] * col) + (r_t_inv[0][1] * row) + (r_t_inv[0][2] * 1); 
   double b = (r_t_inv[1][0] * col) + (r_t_inv[1][1] * row) + (r_t_inv[1][2] * 1); 
   double c = (r_t_inv[2][0] * col) + (r_t_inv[2][1] * row) + (r_t_inv[2][2] * 1); 

   a = a / c;
   b = b / c;

   *x = b;  //x-axis is away from robot front
   *y = a;  //y-axis is toward robot left
}

void Navigation::set_action_client(MoveBaseClient* ac) {
   a = ac;
}

//update the transform from base_link to the goal
void Navigation::update_goal_transform() {
   //compute the goal
   //
   update_goal_mutex.lock();

   int goal_x = int(640 * goal[0]); //get the x of the goal
   int goal_y = int(360 * goal[1]); //get the y of the goal

   int boundary_index = int(goal_x / 8);
   double goal_dist = ground[boundary_index];
   int boundary_y = int(goal_dist * 360.0);
   int ground_y_coord;
   
   if (goal_y > boundary_y) { //if the goal is closer than the ground boundary
      ground_y_coord = goal_y;
   } else {
      ground_y_coord = boundary_y;
   } 

   double x,y;
   convert_image_to_world(goal_x, ground_y_coord, &x, &y);

   //testing
   static tf2_ros::TransformBroadcaster tfb;
   geometry_msgs::TransformStamped transformStamped;

   transformStamped.header.frame_id = "base_link";
   transformStamped.child_frame_id = "goal";
   transformStamped.transform.translation.x = x; //move back .60 meters from the boundary
   transformStamped.transform.translation.y = y;
   transformStamped.transform.translation.z = 0.0;

   transformStamped.transform.rotation.x = 0; //goal should point in same direction as base_link
   transformStamped.transform.rotation.y = 0;
   transformStamped.transform.rotation.z = 0;
   transformStamped.transform.rotation.w = 1;

   transformStamped.header.stamp = ros::Time::now();
   tfb.sendTransform(transformStamped);

   update_goal_mutex.unlock();
}

void Navigation::send_goal(const std_msgs::Empty::ConstPtr& msg) {
   update_goal_mutex.lock();

   //get the transform from odom to goal since the planner only takes goals in 
   //the odom frame
   geometry_msgs::TransformStamped transformStamped;
   transformStamped = tfBuffer.lookupTransform("odom", "goal", ros::Time(0), ros::Duration(.5));

   move_base_msgs::MoveBaseGoal cur_goal;

   //fill in the fields of the goal
   cur_goal.target_pose.header.frame_id = "odom";  //goal must be in the odom frame
   cur_goal.target_pose.header.stamp = ros::Time::now();

   cur_goal.target_pose.pose.position.x = transformStamped.transform.translation.x;
   cur_goal.target_pose.pose.position.y = transformStamped.transform.translation.y;
   cur_goal.target_pose.pose.position.z = 0;
   cur_goal.target_pose.pose.orientation.w = transformStamped.transform.rotation.w;
   cur_goal.target_pose.pose.orientation.x = transformStamped.transform.rotation.x;
   cur_goal.target_pose.pose.orientation.y = transformStamped.transform.rotation.y;
   cur_goal.target_pose.pose.orientation.z = transformStamped.transform.rotation.z;

   /* ROS_INFO("Sending goal"); */
   a->sendGoal(cur_goal);

   update_goal_mutex.unlock();
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

   double a,b;
   double x,y;
   double next_a, next_b;

   x = 0;
   y = ground[0] * 360;
   convert_image_to_world(x, y, &b, &a);

   for (int i=0; i<(numpoints-1); i++)
   {
      double next_x = (i+1)*8;
      double next_y = ground[i+1] * 360;
      double scale = 0.0;

      convert_image_to_world(next_x, next_y, &next_b, &next_a);

      //this loop computes the interpolated point cloud points
      for (int j=0; j<4; j++) {
         *out_x = b + ((next_b - b) * scale); //positive x-axis is forward from robot
         *out_y = a + ((next_a - a) * scale); //positive y-axis is to the left of robot
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

         scale += 1.0/4.0;
      }

      a = next_a;
      b = next_b;
      x = next_x;
      y = next_y;
   }

   pointcloud_pub_.publish(cloud_msg);
}

void Navigation::set_narrow_parameters() {
   dynamic_reconfigure::ReconfigureRequest srv_req;
   dynamic_reconfigure::ReconfigureResponse srv_resp;
   dynamic_reconfigure::DoubleParameter double_param;
   dynamic_reconfigure::Config conf;

   double_param.name = "sim_time";
   double_param.value = 2.0;
   conf.doubles.push_back(double_param);

   srv_req.config = conf;

   ros::service::call("/move_base/DWAPlannerROS/set_parameters", srv_req, srv_resp);
}

void Navigation::execute_turn() {
   diag_timer.stop(); //stop the goal update timer

   update_goal_mutex.lock();

   //get the transform from odom to goal since the planner only takes goals in 
   //the odom frame
   geometry_msgs::TransformStamped transformStamped;
   transformStamped = tfBuffer.lookupTransform("odom", "base_link", ros::Time(0), ros::Duration(.5));

   move_base_msgs::MoveBaseGoal cur_goal;

   //fill in the fields of the goal
   cur_goal.target_pose.header.frame_id = "odom";  //goal must be in the odom frame
   cur_goal.target_pose.header.stamp = ros::Time::now();

   cur_goal.target_pose.pose.position.x = transformStamped.transform.translation.x + 1.0;
   cur_goal.target_pose.pose.position.y = transformStamped.transform.translation.y + 1.0;
   cur_goal.target_pose.pose.position.z = 0;
   cur_goal.target_pose.pose.orientation.w = transformStamped.transform.rotation.w;
   cur_goal.target_pose.pose.orientation.x = transformStamped.transform.rotation.x;
   cur_goal.target_pose.pose.orientation.y = transformStamped.transform.rotation.y;
   cur_goal.target_pose.pose.orientation.z = transformStamped.transform.rotation.z;

   a->sendGoal(cur_goal);

   update_goal_mutex.unlock();
}

void Navigation::autonomous_mode_callback(const std_msgs::Int8::ConstPtr& msg) {
   if (msg->data == 0) {
      //cancel goals
      a->cancelAllGoals();
   }
}

void Navigation::update_goal_callback(const ros::TimerEvent& ev) {
   bool autonomous_mode;

   nh.getParam("/autonomous_mode", autonomous_mode);

   if (autonomous_mode == true) {
      std_msgs::Empty m;
      send_goal((const std_msgs::Empty::ConstPtr&)m);
   }
}

