#include "navigation.h"

#include <dynamic_reconfigure/DoubleParameter.h>
#include <dynamic_reconfigure/Reconfigure.h>
#include <dynamic_reconfigure/Config.h>

#include <pcl_conversions/pcl_conversions.h>
#include <sensor_msgs/PointCloud2.h>
//#include "output.h"

extern int sim_mode;
extern ros::Timer goal_timer;

using namespace std;
typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

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
   /* double r_t_inv[3][3] = {{-0.00266937920376545,-0.00011232742977799345,0.8538495118346788}, */
   /*    {3.7348555556903726e-05,-0.002714164927900428,1.1541406814343038}, */
   /*    {-2.1784034424370855e-05,0.0033593149732969742,-0.10899350363712738}}; */

   //9/23/21 calibration
   double r_t_inv[3][3] = {{-0.0029710385858653108,-2.0372969150804934e-05,0.9260424951167465},
      {1.4259051739232575e-05,-0.00253422318646511,1.2048751880635866},
      {-6.136870472003728e-05,0.00437942965513288,-0.22477696834779506}};

   double a = (r_t_inv[0][0] * col) + (r_t_inv[0][1] * row) + (r_t_inv[0][2] * 1); 
   double b = (r_t_inv[1][0] * col) + (r_t_inv[1][1] * row) + (r_t_inv[1][2] * 1); 
   double c = (r_t_inv[2][0] * col) + (r_t_inv[2][1] * row) + (r_t_inv[2][2] * 1); 

   a = a / c;
   b = b / c;

   *x = b;  //x-axis is away from robot front
   *y = a;  //y-axis is toward robot left

   //testing lookup table
   /* int lookup_table = 0; */

   /* if (lookup_table == 1) { */
   /*    if (row < 0) */
   /*       row = 0; */
   /*    else if (row > 359) */
   /*       row = 359; */

   /*    if (col < 0) */
   /*       col = 0; */
   /*    else if (col > 639) */
   /*       col = 639; */

   /*    *x = front[row][col] * .0254; */
   /*    *y = side[row][col] * .0254; */
   /* } */
}

void Navigation::set_action_client(MoveBaseClient* ac) {
   action_client = ac;
}

//update the transform from base_link to the goal
void Navigation::update_goal_transform() {
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

   if (ground_y_coord < 180) {
      ground_y_coord += 15; //move closer by 5 pixels
   } else {
      ground_y_coord += 10; //move closer by 5 pixels
   }

   //limit the goal Y value
   if (ground_y_coord > 340) {
      ground_y_coord = 340;
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

int Navigation::get_turn_dir(int temp_loc_estimate) {
   int direction;

   if (temp_loc_estimate != -1) {
      //this is for testing turn directions
      if (temp_loc_estimate == 0) { //FIXME
         direction = 0; //left
      } else if (temp_loc_estimate == 1) {
         direction = 2; //right
      } else if (temp_loc_estimate == 4) {
         direction = 2; //right
      } else if (temp_loc_estimate == 5) {
         direction = 2; //right
      } else if (temp_loc_estimate == 6) {
         direction = 0; //left
      } else if (temp_loc_estimate == 7) {
         direction = 0; //left
      } else if (temp_loc_estimate == 8) {
         direction = 0; //left
      } else if (temp_loc_estimate == 9) {
         direction = 2; //right
      } else if (temp_loc_estimate == 10) {
         direction = 2; //right
      } else if (temp_loc_estimate == 11) {
         direction = 2; //right
      } else if (temp_loc_estimate == 12) {
         direction = 0; //left
      } else if (temp_loc_estimate == 14) {
         direction = 2; //right
      } else if (temp_loc_estimate == 15) {
         direction = 0; //left
      } else if (temp_loc_estimate == 16) {
         direction = 0; //left
      } else if (temp_loc_estimate == 17) {
         direction = 2; //right
      } else {
         direction = 0; //default left
      }
   } else {
      return -1;
   }

   return direction;
}

void Navigation::update_turn_transform() {
   geometry_msgs::TransformStamped *t;
   int temp_loc_estimate;

   while(1) {
      cur_loc_mutex.lock();
      temp_loc_estimate = cur_loc_estimate;  //read the current hallway estimate
      cur_loc_mutex.unlock();

      if (temp_loc_estimate != -1) {
         int direction;

         //this is for testing turn directions
         if (temp_loc_estimate == 0) { //FIXME
            direction = 0; //left
         } else if (temp_loc_estimate == 1) {
            direction = 2; //right
         } else if (temp_loc_estimate == 4) {
            direction = 2; //right
         } else if (temp_loc_estimate == 5) {
            direction = 2; //right
         } else if (temp_loc_estimate == 6) {
            direction = 0; //left
         } else if (temp_loc_estimate == 7) {
            direction = 0; //left
         } else if (temp_loc_estimate == 8) {
            direction = 0; //left
         } else if (temp_loc_estimate == 9) {
            direction = 2; //right
         } else if (temp_loc_estimate == 10) {
            direction = 2; //right
         } else if (temp_loc_estimate == 11) {
            direction = 2; //right
         } else if (temp_loc_estimate == 12) {
            direction = 0; //left
         } else if (temp_loc_estimate == 14) {
            direction = 2; //right
         } else if (temp_loc_estimate == 15) {
            direction = 0; //left
         } else if (temp_loc_estimate == 16) {
            direction = 0; //left
         } else if (temp_loc_estimate == 17) {
            direction = 2; //right
         } else {
            direction = 0; //default left
         }

         if (direction == 0) { //turn left at this hallway
            t = &(turn_transform_left[temp_loc_estimate]);
         } else { //turn right at this hallway
            t = &(turn_transform_right[temp_loc_estimate]);
         }

         static tf2_ros::TransformBroadcaster tfb;
         geometry_msgs::TransformStamped transformStamped;

         transformStamped.header.frame_id = "base_link";
         transformStamped.child_frame_id = "turn";
         transformStamped.transform.translation.x = t->transform.translation.x; 
         transformStamped.transform.translation.y = t->transform.translation.y;
         transformStamped.transform.translation.z = t->transform.translation.z;

         transformStamped.transform.rotation.w = t->transform.rotation.w; 
         transformStamped.transform.rotation.x = t->transform.rotation.x; 
         transformStamped.transform.rotation.y = t->transform.rotation.y;
         transformStamped.transform.rotation.z = t->transform.rotation.z; 

         transformStamped.header.stamp = ros::Time::now();
         tfb.sendTransform(transformStamped);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }
}

void Navigation::send_goal(const std_msgs::Empty::ConstPtr& msg) {
   //get the transform from odom to goal since the planner only takes goals in 
   //the odom frame
   geometry_msgs::TransformStamped transformStamped;
   transformStamped = tfBuffer.lookupTransform("odom", "goal", ros::Time(0), ros::Duration(0.5));

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
   update_goal_mutex.lock();
   action_client->sendGoal(cur_goal);
   update_goal_mutex.unlock();
}

void Navigation::publish_pointcloud() {
   int numpoints = 80; //the number of points output by the neural network
   int interpolate = 15; //number of points to interpolate between 2 neural network (world) points

   sensor_msgs::PointCloud2 cloud_msg;

   cloud_msg.header.frame_id = "see3_cam";
   cloud_msg.header.stamp = ros::Time::now();
   cloud_msg.width  = (numpoints-1)*interpolate; //number of points (interpolate 4 times the points)
   cloud_msg.height = 1;
   cloud_msg.is_bigendian = false;
   cloud_msg.is_dense = true; // there may be invalid points

   sensor_msgs::PointCloud2Modifier modifier(cloud_msg);
   modifier.setPointCloud2FieldsByString(2,"xyz","rgb");
   modifier.resize((numpoints-1)*interpolate); //interpolate 4 times the points

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
      for (int j=0; j<interpolate; j++) {
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

         scale += 1.0/interpolate;
      }

      a = next_a;
      b = next_b;
      x = next_x;
      y = next_y;
   }

   pointcloud_pub_.publish(cloud_msg);
}

//set the planning parameters when traversing a narrow hall
void Navigation::set_narrow_parameters(int narrow) {
   dynamic_reconfigure::ReconfigureRequest srv_req;
   dynamic_reconfigure::ReconfigureResponse srv_resp;
   dynamic_reconfigure::DoubleParameter double_param;
   dynamic_reconfigure::Config conf;

   //when narrow, set the sim_time to short so that the local planner
   //will closely follow the global plan
   double_param.name = "sim_time";

   if (narrow == 1) {
      double_param.value = 2.2;
   } else {
      double_param.value = 2.8;
   }

   conf.doubles.push_back(double_param);

   //when narrow, set the path_distance_bias so that the local planner
   //will closely follow the global plan
   //path_distance_bias: .6
   double_param.name = "path_distance_bias";

   if (narrow == 1) {
      double_param.value = .8;
   } else {
      double_param.value = .6;
   }

   conf.doubles.push_back(double_param);

   //when narrow, set the maximum forward velocity to slower
   //max_vel_x: 0.23
   double_param.name = "max_vel_x";

   if (narrow == 1) {
      double_param.value = .18;
   } else {
      double_param.value = .31;
   }

   conf.doubles.push_back(double_param);

   srv_req.config = conf;

   ros::service::call("/move_base/DWAPlannerROS/set_parameters", srv_req, srv_resp);

   //change the footprint size when narrow
   /* double_param.name = "footprint_padding"; */

   /* if (narrow == 1) { */
   /*    double_param.value = .16; */
   /* } else { */
   /*    double_param.value = .26; */
   /* } */

   /* conf.doubles.clear(); */
   /* conf.doubles.push_back(double_param); */
   /* srv_req.config = conf; */
   /* ros::service::call("/move_base/local_costmap/set_parameters", srv_req, srv_resp); */

   /* double_param.name = "footprint_padding"; */

   /* if (narrow == 1) { */
   /*    double_param.value = .16; */
   /* } else { */
   /*    double_param.value = .26; */
   /* } */

   /* conf.doubles.clear(); */
   /* conf.doubles.push_back(double_param); */
   /* srv_req.config = conf; */
   /* ros::service::call("/move_base/global_costmap/set_parameters", srv_req, srv_resp); */

   /* double_param.name = "planner_frequency"; */

   /* if (narrow == 1) { */
   /*    double_param.value = 1.0; */
   /* } else { */
   /*    double_param.value = 0.5; */
   /* } */

   /* conf.doubles.push_back(double_param); */

   /* srv_req.config = conf; */

   /* ros::service::call("/move_base/set_parameters", srv_req, srv_resp); */
   //ros::service::call("/move_base/local_costmap/set_parameters", srv_req, srv_resp);
}

//parameters when turning
void Navigation::set_turn_parameters() {
   dynamic_reconfigure::ReconfigureRequest srv_req;
   dynamic_reconfigure::ReconfigureResponse srv_resp;
   dynamic_reconfigure::DoubleParameter double_param;
   dynamic_reconfigure::Config conf;

   //when turning, do not closely follow the global plan
   double_param.name = "sim_time";
   double_param.value = 2.8;

   conf.doubles.push_back(double_param);

   //when turning, do not closely follow the global plan
   //path_distance_bias: .6
   double_param.name = "path_distance_bias";
   double_param.value = .6;

   conf.doubles.push_back(double_param);

   //when turning, set the maximum forward velocity to slower
   //max_vel_x: 0.23
   double_param.name = "max_vel_x";
   double_param.value = .18;

   conf.doubles.push_back(double_param);

   srv_req.config = conf;

   ros::service::call("/move_base/DWAPlannerROS/set_parameters", srv_req, srv_resp);
}

int Navigation::execute_turn2(int reset_turn) {
   static int turn_start = 0;
   int frame_skip = 3; //skip every few frames, so this is not run every frame
   double distance_forward = 1.0;
   static double start_x;
   static double start_y;
   static int straight = 0;
   static int rotate = 0;
   static int start_hallway = -1, end_hallway, turn_dir;
   static float start_turn_heading;
   geometry_msgs::Twist msg;
   geometry_msgs::TransformStamped *t;

   if (reset_turn) { //reset the status of a turn
      turn_start = 0;
      return 0;
   }

   if (turn_start % frame_skip == 0) { 
      if (turn_start == 0) {
         //record the start x and y of the turn
         start_x = localization_pose[localization_index].position.x;
         start_y = localization_pose[localization_index].position.y;

         start_hallway = cur_loc_estimate; //record the starting hallway number

         if (start_hallway != -1) { //if good localization reading, then update other settings
            turn_dir = get_turn_dir(start_hallway);

            if (turn_dir == 0) { //left turn
               t = &(turn_transform_left[start_hallway]);
            } else { //right turn
               t = &(turn_transform_right[start_hallway]);
            }

            //get the ending hallway number when the turn completes
            end_hallway = get_next_hallway_num(start_hallway,turn_dir);
         }

         straight = 1;
      }

      if ((start_hallway == -1) && (cur_loc_estimate != -1)) { //keep reading the hallway estimate until known
         start_hallway = cur_loc_estimate;
         turn_dir = get_turn_dir(start_hallway);

         if (turn_dir == 0) { //left turn
            t = &(turn_transform_left[start_hallway]);
         } else { //right turn
            t = &(turn_transform_right[start_hallway]);
         }

         end_hallway = get_next_hallway_num(start_hallway,turn_dir);
      }
      
      //compute the average of the goal x/y position
      int num_goal_average = 4; //number of goal points to average across
      float avg_goal_x = 0;
      float avg_goal_y = 0;
      int temp_goal_index = goal_cur_index;

      for (int i=0; i<num_goal_average; i++) {
         avg_goal_x += goal_array_x[temp_goal_index];
         avg_goal_y += goal_array_y[temp_goal_index];

         temp_goal_index--;

         if (temp_goal_index < 0) {
            temp_goal_index = GOAL_ARRAY_SIZE - 1;
         }
      }

      avg_goal_x /= num_goal_average;
      avg_goal_y /= num_goal_average;

      avg_goal_x *= 640.0;
      avg_goal_y *= 360.0;
      
      if (straight == 1) { //start turn by going straight a fixed distance
         //check for obstacles
         int found_obstacle=0;

         int ground_obstacle_width = 12;  //use +/- 12 points around the center of the image
         for(int i=(40-ground_obstacle_width); i<(40+ground_obstacle_width); i++) {
            if (ground[i] > 0.7) {  // 252/360 = .7 - no obstacle in lower 30% of image
               found_obstacle = 1;
            }
         }

         //if there is an obstacle, then stop
         if (found_obstacle) {
            msg.linear.x = 0.0; //set the linear velocity
            msg.angular.z = 0.0;

            //publish the message and return
            twist_pub_.publish(msg); //publish the twist message

            return 0; //turn still in progress
         }

         //move forward while computing the turn towards the goal 
         int midpoint_x = 320;
         float Kp = .002;
         float ang_vel, max_ang_vel=.4;

         ang_vel = Kp * (midpoint_x - avg_goal_x);

         if (ang_vel > max_ang_vel) {
            ang_vel = max_ang_vel;
         } else if (ang_vel < -max_ang_vel) {
            ang_vel = -max_ang_vel;
         }

         msg.linear.x = 0.18; //set the linear velocity
         msg.angular.z = ang_vel; //set the angular velocity to rotate towards the goal

         //compute the current distance traveled
         float cur_distance_forward;
         float cur_x = localization_pose[localization_index].position.x;
         float cur_y = localization_pose[localization_index].position.y;

         cur_distance_forward = sqrt(pow(cur_x - start_x,2) + pow(cur_y - start_y,2));

         if(cur_distance_forward < distance_forward) {
            //publish the message and return
            twist_pub_.publish(msg); //publish the twist message

            return 0; //turn still in progress
         } else { //done going straight
            msg.linear.x = 0.0; //set the linear velocity
            msg.angular.z = 0.0;
            twist_pub_.publish(msg); //publish the twist message
         }

         //done with going straight
         straight = 0;

         heading_mutex.lock();
         start_turn_heading = actual_heading;
         heading_mutex.unlock();
      } else { //not going straight, but rotating now
         //move forward is complete, so stop and rotate in place
         rotate = 1;

         //make the rotate in place turn
         if (turn_dir == 0) { //turn left
            float cur_heading;

            msg.linear.x = 0.0; //set the linear velocity
            msg.angular.z = 0.15;
            twist_pub_.publish(msg); //publish the twist message

            heading_mutex.lock();
            cur_heading = actual_heading;
            heading_mutex.unlock();

            if (cur_heading > (start_turn_heading + 90.0)) {
               //stop turning if turned more than 90 degrees
               msg.linear.x = 0.0; //set the linear velocity
               msg.angular.z = 0.0;
               twist_pub_.publish(msg); //publish the twist message

               //the turn is complete
               start_hallway = -1;
               turn_start = 0;
               return 1; //turn complete
            }
         } else if (turn_dir == 2) { //turn right
            float cur_heading;

            msg.linear.x = 0.0; //set the linear velocity
            msg.angular.z = -0.15;
            twist_pub_.publish(msg); //publish the twist message

            heading_mutex.lock();
            cur_heading = actual_heading;
            heading_mutex.unlock();

            if (cur_heading < (start_turn_heading - 90.0)) {
               //stop turning if turned more than 90 degrees
               msg.linear.x = 0.0; //set the linear velocity
               msg.angular.z = 0.0;
               twist_pub_.publish(msg); //publish the twist message
               
               //the turn is complete
               start_hallway = -1;
               turn_start = 0;
               return 1; //turn complete
            }
         } //end right turn
      } //end rotation section
   }

   turn_start++; //increment the turn frame counter
   return 0; //turn still in progress
}

void Navigation::execute_turn() {
   goal_timer.stop(); //stop the goal update timer

   update_goal_mutex.lock();

   //get the transform from odom to goal since the planner only takes goals in 
   //the odom frame
   geometry_msgs::TransformStamped transformStamped;
   transformStamped = tfBuffer.lookupTransform("odom", "turn", ros::Time(0), ros::Duration(1.5));

   move_base_msgs::MoveBaseGoal temp_goal;

   //fill in the fields of the goal
   temp_goal.target_pose.header.frame_id = "odom";  //goal must be in the odom frame
   temp_goal.target_pose.header.stamp = ros::Time::now();

   temp_goal.target_pose.pose.position.x = transformStamped.transform.translation.x;
   temp_goal.target_pose.pose.position.y = transformStamped.transform.translation.y;
   temp_goal.target_pose.pose.position.z = 0;
   
   ROS_WARN("JS turn goal");
   ROS_WARN("JS turn goal x: %f", temp_goal.target_pose.pose.position.x);
   ROS_WARN("JS turn goal y: %f", temp_goal.target_pose.pose.position.y);
   std::cout << "JS turn goal x: " << temp_goal.target_pose.pose.position.x << std::endl;
   std::cout << "JS turn goal y: " << temp_goal.target_pose.pose.position.y << std::endl;
   
   //save the current turn goal
   cur_goal.target_pose.pose.position.x = transformStamped.transform.translation.x;
   cur_goal.target_pose.pose.position.y = transformStamped.transform.translation.y;
   cur_goal.target_pose.pose.position.z = 0;

   temp_goal.target_pose.pose.orientation.w = transformStamped.transform.rotation.w;
   temp_goal.target_pose.pose.orientation.x = transformStamped.transform.rotation.x;
   temp_goal.target_pose.pose.orientation.y = transformStamped.transform.rotation.y;
   temp_goal.target_pose.pose.orientation.z = transformStamped.transform.rotation.z;

   //save the current turn orientation
   cur_goal.target_pose.pose.orientation.w = transformStamped.transform.rotation.w;
   cur_goal.target_pose.pose.orientation.x = transformStamped.transform.rotation.x;
   cur_goal.target_pose.pose.orientation.y = transformStamped.transform.rotation.y;
   cur_goal.target_pose.pose.orientation.z = transformStamped.transform.rotation.z;

   action_client->sendGoal(temp_goal);

   update_goal_mutex.unlock();
}

//return the distance from the current position to the goal
double Navigation::get_distance_to_goal() {
  geometry_msgs::TransformStamped transformStamped;

  try{
    transformStamped = tfBuffer.lookupTransform("odom", "base_link", ros::Time(0));
  }
  catch (tf2::TransformException &ex) {
    ROS_WARN("%s",ex.what());
    return -1;  //if error, return -1
  }

  double x_disp = transformStamped.transform.translation.x;
  double y_disp = transformStamped.transform.translation.y;

  double x_dest = cur_goal.target_pose.pose.position.x;
  double y_dest = cur_goal.target_pose.pose.position.y;

  /* ROS_INFO("xdisp: %f xdest: %f y_disp: %f y_dest: %f", x_disp, x_dest, y_disp, y_dest); */
  double dist = pow((pow(x_dest - x_disp, 2) + pow(y_dest - y_disp, 2)), .5);
  return dist;
}

void Navigation::autonomous_mode_callback(const std_msgs::Int8::ConstPtr& msg) {
   if (msg->data == 0) {
      //cancel goals
      update_goal_mutex.lock();
      action_client->cancelAllGoals();
      update_goal_mutex.unlock();
   }
}

void Navigation::update_goal_callback(const ros::TimerEvent& ev) {
   bool autonomous_mode;

   nh.getParam("/autonomous_mode", autonomous_mode);

   if (autonomous_mode == true) {
      int temp_loc_estimate; 

      cur_loc_mutex.lock();
      temp_loc_estimate = cur_loc_estimate;  //read the current hallway estimate
      cur_loc_mutex.unlock();

      if (temp_loc_estimate != -1) {
         update_goal_mutex.lock();
         int narrow = VAN(&gr, "narrow", cur_loc_estimate);
         set_narrow_parameters(narrow);
         update_goal_mutex.unlock();
      }

      std_msgs::Empty m;
      send_goal((const std_msgs::Empty::ConstPtr&)m);
   }
}

//set transform array entry
void Navigation::set_turn_entry(int hallway_num, double x, double y, double degrees) {
   //hallway_num is the hallway the robot is currently in
   //x is the distance in meters forward
   //y is the distance in meters sideways (positive to the left)
   tf2::Quaternion tempQuaternion;
   geometry_msgs::TransformStamped *t;

   if (degrees > 0) { //positive degrees is counterclockwise
      t = &(turn_transform_left[hallway_num]);
   } else { //negative degrees is clockwise
      t = &(turn_transform_right[hallway_num]);
   }

   tempQuaternion.setRPY(0,0,degrees*M_PI/180.0); 
   tempQuaternion=tempQuaternion.normalize();

   t->transform.translation.x = x; 
   t->transform.translation.y = y;
   t->transform.translation.z = 0.0;
   t->transform.rotation.x = tempQuaternion.x(); 
   t->transform.rotation.y = tempQuaternion.y();
   t->transform.rotation.z = tempQuaternion.z(); 
   t->transform.rotation.w = tempQuaternion.w();
}

//initialize the turn transform arrays
void Navigation::init_turn_transforms() {
   tf2::Quaternion tempQuaternion;
   geometry_msgs::TransformStamped *t;

   //zero out all transforms
   for(int i=0; i<64; i++) {
      t = &(turn_transform_left[i]);

      t->transform.translation.x = 0; 
      t->transform.translation.y = 0;
      t->transform.translation.z = 0;
      t->transform.rotation.x = 0; 
      t->transform.rotation.y = 0;
      t->transform.rotation.z = 0; 
      t->transform.rotation.w = 1;

      t = &(turn_transform_right[i]);

      t->transform.translation.x = 0; 
      t->transform.translation.y = 0;
      t->transform.translation.z = 0;
      t->transform.rotation.x = 0; 
      t->transform.rotation.y = 0;
      t->transform.rotation.z = 0; 
      t->transform.rotation.w = 1;
   }


   //clockwise loop
   set_turn_entry(8, 1.9, .3, 85); //+90 degrees is counter-clockwise
   set_turn_entry(11, 1.1, 0.0, -90);
   set_turn_entry(12, 2.0, 0, 90);
   set_turn_entry(15, 1.8, 0, 90);
   set_turn_entry(16, 1.9, 0, 92);
   set_turn_entry(0, 1.0, -0.1, 90);
   set_turn_entry(4, 1.5, 0, -90);
   set_turn_entry(7, 1.7, 0, 90);

   set_turn_entry(6, 1.0, 0, 90);
   set_turn_entry(5, 2.0, 0, -90);
   set_turn_entry(1, 1.8, 0, -90);
   set_turn_entry(17, 1.8, 0, -90);
   set_turn_entry(14, 1.0, 0.1, -90);
   set_turn_entry(13, 1.5, 0, 90);
   set_turn_entry(10, 1.2, 0, -90);
   set_turn_entry(9, 1.6, 0, -90);

   //make entry for 180 degree turn
   set_turn_entry(31, .1, 0, 180);
}

//initialize the route
void Navigation::init_route() {
   //route for a loop around north quad
   route_hallway[0] = 8;
   route_turn[0] = 0; //left

   route_hallway[1] = 11;
   route_turn[1] = 2; //right

   route_hallway[2] = 12;
   route_turn[2] = 0; //left

   route_hallway[3] = 15;
   route_turn[3] = 0; //left

   route_hallway[4] = 16;
   route_turn[4] = 0; //left

   route_hallway[5] = 0;
   route_turn[5] = 0; //left

   route_hallway[6] = 4;
   route_turn[6] = 2; //right

   route_hallway[7] = 7;
   route_turn[7] = 0; //left
}

void Navigation::turn_degrees(int degrees) {
   float new_heading;
   float desired_heading;
   float angular_velocity;
   geometry_msgs::Twist msg;

   heading_mutex.lock();
   new_heading = actual_heading;
   heading_mutex.unlock();

   desired_heading = new_heading + degrees;

   if (desired_heading > new_heading) { //turn counterclockwise
      angular_velocity = .9;

      msg.linear.x = 0;
      msg.angular.z = angular_velocity;

      while(new_heading < desired_heading) {
         heading_mutex.lock();
         new_heading = actual_heading;
         heading_mutex.unlock();
         
         twist_pub_.publish(msg); //publish the twist message
         
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      //stop
      msg.linear.x = 0;
      msg.angular.z = 0;
      twist_pub_.publish(msg); //publish the twist message
   } else { //turn clockwise
      angular_velocity = -.9;

      msg.linear.x = 0;
      msg.angular.z = angular_velocity;
   }
}

int Navigation::call_make_plan(double goal_x, double goal_y, double* pose_x, double* pose_y) {
   ros::ServiceClient serviceClient = nh.serviceClient<nav_msgs::GetPlan>(service_name);
   nav_msgs::GetPlan srv;

   //get the transform of the base_link
   geometry_msgs::TransformStamped transformStamped;
   transformStamped = tfBuffer.lookupTransform("odom", "base_link", ros::Time(0));

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
