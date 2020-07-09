#include "encoder_odom.h"

#define MAX_MOTOR_SPEED 28000

using namespace std;

extern ros::NodeHandle *nh;
extern ros::Subscriber sub_, sub_stop;
extern ros::Subscriber rtabmap_info_sub;
extern ros::Publisher pub_, loop_closure_pub;
extern ros::Publisher twist_pub;

void OdometryPublisher::rtabmap_info_callback(const rtabmap_ros::Info::ConstPtr& info) {
  //this function is called when a message is published to /rtabmap/info
  std_msgs::Int32MultiArray a;

  a.data.clear();

  a.data.push_back(info->loopClosureId);
  a.data.push_back(info->proximityDetectionId);

  if (rtabmap_started == 0) {
    rtabmap_started = 1;
  }

  loop_closure_pub.publish(a);
}

void OdometryPublisher::publish_odometry_message(double vx, double vth) {
  //publish a new odometry message
  tf2::Quaternion odom_quat;
  odom_quat.setRPY(0,0,cur_theta);
  current_time = ros::Time::now();

  geometry_msgs::TransformStamped odom_trans;
  odom_trans.header.stamp = current_time;
  odom_trans.header.frame_id = "odom";
  odom_trans.child_frame_id = "roboclaw_center";

  odom_trans.transform.translation.x = cur_x;
  odom_trans.transform.translation.y = cur_y;
  odom_trans.transform.translation.z = 0.0;
  odom_trans.transform.rotation.x = odom_quat.x();
  odom_trans.transform.rotation.y = odom_quat.y();
  odom_trans.transform.rotation.z = odom_quat.z();
  odom_trans.transform.rotation.w = odom_quat.w();
  
  //send the transform
  odom_broadcaster.sendTransform(odom_trans);
  
  //create the Odometry message
  nav_msgs::Odometry odom;
  odom.header.stamp = current_time;
  odom.header.frame_id = "odom";
  odom.child_frame_id = "roboclaw_center";

  //set the position
  odom.pose.pose.position.x = cur_x;
  odom.pose.pose.position.y = cur_y;
  odom.pose.pose.position.z = 0.0;
  odom.pose.pose.orientation.x = odom_trans.transform.rotation.x;
  odom.pose.pose.orientation.y = odom_trans.transform.rotation.y;
  odom.pose.pose.orientation.z = odom_trans.transform.rotation.z;
  odom.pose.pose.orientation.w = odom_trans.transform.rotation.w;

  odom.pose.covariance[0] = 0.01;
  odom.pose.covariance[7] = 0.01;
  odom.pose.covariance[14] = 10; //z velocity
  odom.pose.covariance[21] = 99999;
  odom.pose.covariance[28] = 99999;
  odom.pose.covariance[35] = 0.01;
  /* odom.pose.covariance[35] = 0.003; */

  //set the velocity
  odom.twist.twist.linear.x = vx;
  odom.twist.twist.linear.y = 0;
  odom.twist.twist.angular.z = vth;

  //set the twist covariance
  odom.twist.covariance = odom.pose.covariance;

  //send the message
  pub_.publish(odom);
  
  ////////////////////////
  //create the Twist message
  geometry_msgs::TwistWithCovarianceStamped t;
  t.header.stamp = current_time;
  t.header.frame_id = "roboclaw_center";

  t.twist.twist.linear.x = vx;
  t.twist.twist.linear.y = 0;
  t.twist.twist.linear.z = 0;
  t.twist.twist.angular.x = 0;
  t.twist.twist.angular.y = 0;
  t.twist.twist.angular.z = vth;
  t.twist.covariance = odom.pose.covariance;
  
  //send the message
  twist_pub.publish(t);
}

void OdometryPublisher::update_odometry(int enc_left, int enc_right, double* vel_x, double* vel_theta) {
  int left_ticks, right_ticks;
  double dist_left, dist_right, dist;
  double d_theta, r;
  //ros::Time current_time = ros::Time::now();

  //take the encoder counts and update the number of ticks traveled
  left_ticks = enc_left - last_enc_left;
  right_ticks = enc_right - last_enc_right;
  last_enc_left = enc_left;
  last_enc_right = enc_right;

  //compute the distance traveled by each wheel
  dist_left = left_ticks / TICKS_PER_METER;
  dist_right = right_ticks / TICKS_PER_METER;
  dist = (dist_right + dist_left) / 2.0;

  //compute elapsed time
  double d_time = (current_time - last_enc_time).toSec();
  last_enc_time = current_time;

  double current_left_vel = left_ticks / d_time; //ticks/second
  double current_right_vel = right_ticks / d_time;

  //if last_left_vel != 0 and last_right_vel != 0:

  //when computing the current tick velocity, filter the readings
  //from the encoders as the readings are noisy at slow speeds
  /* left_tick_vel =  .3*left_tick_vel + .7*(current_left_vel); //should be in ticks/second */
  /* right_tick_vel = .3*right_tick_vel + .7*(current_right_vel); */
  left_tick_vel =  (current_left_vel); //should be in ticks/second
  right_tick_vel = (current_right_vel);

  if (debug_odometry == 1) {
    ROS_INFO("--update odometry --- Current tick velocity: left: %f, right: %f",  left_tick_vel, right_tick_vel);
  }

  // TODO find better what to determine going straight,
  // this means slight deviation is accounted
  if (left_ticks == right_ticks) {
    d_theta = 0.0;
    cur_x += dist * cos(cur_theta);
    cur_y += dist * sin(cur_theta);
  } else {
    //flip sign of theta
    d_theta = -(dist_right - dist_left) / BASE_WIDTH;
    r = dist / d_theta;
    cur_x += r * (sin(d_theta + cur_theta) - sin(cur_theta));
    cur_y -= r * (cos(d_theta + cur_theta) - cos(cur_theta));
    cur_theta = normalize_angle(cur_theta - d_theta);
  }

  if (abs(d_time) < 0.000001) {
    *vel_x = 0.0;
    *vel_theta = 0.0;
  } else {
    *vel_x = dist / d_time;
    //flip the sign of theta
    *vel_theta = -d_theta / d_time;
    //ROS_INFO("--debugging --- d_theta: %f, d_time: %f",  d_theta, d_time);
  }

  return;
}

void OdometryPublisher::encoder_message_callback(const std_msgs::Int32MultiArray::ConstPtr& enc_msg) {
  //this callback is called when a new encoder message is
  //received from the Arduino on the /encoder_service topic
  int enc_left, enc_right;
  double vel_x, vel_theta;

  enc_left = enc_msg->data[0];
  enc_right = enc_msg->data[1];

  //ROS_INFO("debugging left: %d, right %d",  enc_left, enc_right);
  //if there is a big jump in the encoder readings, then ignore the reading
  if (abs(enc_left - last_enc_left) > 2500) {
    ROS_INFO("Ignoring left encoder jump: cur %d, last %d",  enc_left, last_enc_left);
    //stop = 1;
    last_enc_left = enc_left;
    last_enc_right = enc_right;
  } else if (abs(enc_right - last_enc_right) > 2500) {
    ROS_INFO("Ignoring right encoder jump: cur %d, last %d", enc_right, last_enc_right);
    //stop = 1;
    last_enc_left = enc_left;
    last_enc_right = enc_right;
  } else {
    current_time = ros::Time::now();

    //if over 20ms has elapsed since the last encoder message, then process
    if ((current_time.toSec() - last_enc_time.toSec()) > .020) {
      //call the update function
      update_odometry(enc_left, enc_right, &vel_x, &vel_theta);
      publish_odometry_message(vel_x, vel_theta);

      run_pid();
    } else {
      ROS_INFO("Encoder delay error.");
    }
  }

}

void OdometryPublisher::run_pid() { 
  double ltv, rtv;
  double des_vel_left, des_vel_right;
  int last_left_motor_cmd=0, last_right_motor_cmd=0; //last motor command

  desired_vel_mutex.lock();
  des_vel_left = (double) desired_vl; //in ticks/sec
  des_vel_right = (double) desired_vr;
  desired_vel_mutex.unlock();

  ltv = left_tick_vel; //actual left velocity in ticks/sec
  rtv = right_tick_vel; //actual right velocity in ticks/sec

  last_left_motor_cmd = cur_left_motor;
  last_right_motor_cmd = cur_right_motor;

  compute_pid(des_vel_left, ltv, des_vel_right, rtv); //compute the new motor speeds

  if (debug_odometry == 1) {
    ROS_INFO("desired vl_ticks: %f desired vr_ticks: %f", des_vel_left, des_vel_right);
    ROS_INFO("current left vel: %f current right vel: %f", \
         ltv, rtv);
    ROS_INFO("left error: %f right error: %f", \
         des_vel_left - ltv, \
         des_vel_right - rtv);

    ROS_INFO("left pid output: %d right pid output: %d", cur_left_motor, cur_right_motor);
  }

  //limit motor acceleration/deceleration
  //without this, the starts/stops are abrupt
  if (abs(cur_left_motor - last_left_motor_cmd) > 400) {
    cur_left_motor = cur_left_motor * .10 + last_left_motor_cmd * .90;
  }

  if (abs(cur_right_motor - last_right_motor_cmd) > 400) {
    cur_right_motor = cur_right_motor * .10 + last_right_motor_cmd * .90;
  }

  //set the motor speeds
  setmotor_mutex.lock();
  setmotor(-cur_left_motor, -cur_right_motor);
  setmotor_mutex.unlock();
}

void OdometryPublisher::compute_pid(double left_desired, double left_actual, double right_desired, double right_actual) {
  double kp = 4.8;
  double ki = .5;
  double kd = 0.30;
  int i;

  //test acceleration
  if ((abs(left_desired) < 1000) || (abs(right_desired) < 1000) || (abs(left_actual) < 1000) || (abs(right_actual) < 1000)) {
  //if ((abs(left_desired) < 1000) || (abs(right_desired) < 1000) ) {
    //kp = 1.0;
  }

  //if the wheel velocities are set to 0
  //if ((abs(left_desired) < .0001 && abs(right_desired) < .0001) || stop == 1) {
  if ((abs(left_desired) < .0001 && abs(right_desired) < .0001) && (1==0)) {
    cur_left_motor = 0;
    cur_right_motor = 0;

    last_left_error = 0;
    last_right_error = 0;

    for (i=0; i<INTEGRAL_ARRAY_SIZE; i++) {
        left_integral[i] = 0;
        right_integral[i] = 0;
    }

    desired_vel_mutex.lock();
    desired_vl = 0;
    desired_vr = 0;
    desired_vel_mutex.unlock();

    left_tick_vel = 0;
    right_tick_vel = 0;

    return;
  }

  double left_error = left_desired - left_actual;
  double right_error = right_desired - right_actual;
  double left_error_diff = last_left_error - left_error;
  double right_error_diff = last_right_error - right_error;

  //compute integral term
  double left_sum = 0.0;
  double right_sum = 0.0;

  left_counter = (left_counter + 1) % INTEGRAL_ARRAY_SIZE;
  right_counter = (right_counter + 1) % INTEGRAL_ARRAY_SIZE;
  left_integral[left_counter] = left_error;
  right_integral[right_counter] = right_error;

  for (i=0; i<INTEGRAL_ARRAY_SIZE; i++) {
    left_sum += left_integral[i];
    right_sum += right_integral[i];
  }

  if (left_sum > 18000) {
    left_sum = 18000;
  } else if (left_sum < -18000) {
    left_sum = -18000;
  }

  if (right_sum > 18000) {
    right_sum = 18000;
  } else if (right_sum < -18000) {
    right_sum = -18000;
  }

  /* *left_set_value = (kp * left_error) + (ki * left_sum) - (kd * left_error_diff); */
  /* *right_set_value = (kp * right_error) + (ki * right_sum) - (kd * right_error_diff); */

  double pl_out, il_out, dl_out;
  double pr_out, ir_out, dr_out;

  pl_out = kp * left_error;
  il_out = ki * left_sum;
  dl_out = kd * left_error_diff;

  pr_out = kp * right_error;
  ir_out = ki * right_sum;
  dr_out = kd * right_error_diff;

  if (debug_odometry == 1) {
    ROS_INFO("P: %f, I: %f, D: %f", pl_out, il_out, dl_out);
  }

  cur_left_motor = pl_out + il_out + dl_out;
  cur_right_motor = pr_out + ir_out + dr_out;

  last_left_error = left_error;
  last_right_error = right_error;

  //limit the max speed
  if (cur_left_motor > MAX_MOTOR_SPEED) {
    cur_left_motor = MAX_MOTOR_SPEED;
  } else if (cur_left_motor < -MAX_MOTOR_SPEED)
    cur_left_motor = -MAX_MOTOR_SPEED;

  if (cur_right_motor > MAX_MOTOR_SPEED) {
    cur_right_motor = MAX_MOTOR_SPEED;
  } else if (cur_right_motor < -MAX_MOTOR_SPEED)
    cur_right_motor = -MAX_MOTOR_SPEED;
}

void OdometryPublisher::stop_toggle_callback(const std_msgs::Empty::ConstPtr&) {
  //stop ^= 1;
  planner ^= 1;

  if (planner == 1) { //running in autonomous mode
    nh->setParam("/autonomous_mode", true);
  } else {
    nh->setParam("/autonomous_mode", false);
  }
}

void OdometryPublisher::cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist) {
  planner_mutex.lock();
  if (planner == 0) {
    //update the time, this is used for the 1 second timeout
    last_set_speed_time_mutex.lock();
    last_set_speed_time = ros::Time::now();
    last_set_speed_time_mutex.unlock();

    double linear_x = twist->linear.x;
    double angular_z = -twist->angular.z;

    if (abs(linear_x) > MAX_ABS_LINEAR_SPEED) {
      if (linear_x > 0)
        linear_x = MAX_ABS_LINEAR_SPEED;
      else 
        linear_x = -MAX_ABS_LINEAR_SPEED;
    }
    if (abs(angular_z) > MAX_ABS_ANGULAR_SPEED) {
      if (angular_z > 0)
        angular_z = MAX_ABS_ANGULAR_SPEED;
      else 
        angular_z = -MAX_ABS_ANGULAR_SPEED;
    }

    double vl = (linear_x + angular_z * BASE_WIDTH / 2.0); //meters/sec
    double vr = (linear_x - angular_z * BASE_WIDTH / 2.0);  

    desired_vel_mutex.lock();
    desired_vl = int(vl * TICKS_PER_METER); //ticks/sec
    desired_vr = int(vr * TICKS_PER_METER);  
    desired_vel_mutex.unlock();
  }
  planner_mutex.unlock();
}

void OdometryPublisher::planner_cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist) {
  planner_mutex.lock();
  if (planner == 1) {
    //update the time, this is used for the 1 second timeout
    last_set_speed_time_mutex.lock();
    last_set_speed_time = ros::Time::now();
    last_set_speed_time_mutex.unlock();

    double linear_x = twist->linear.x;
    double angular_z = -twist->angular.z;

    if (abs(linear_x) > MAX_ABS_LINEAR_SPEED) {
      if (linear_x > 0)
        linear_x = MAX_ABS_LINEAR_SPEED;
      else 
        linear_x = -MAX_ABS_LINEAR_SPEED;
    }
    if (abs(angular_z) > MAX_ABS_ANGULAR_SPEED) {
      if (angular_z > 0)
        angular_z = MAX_ABS_ANGULAR_SPEED;
      else 
        angular_z = -MAX_ABS_ANGULAR_SPEED;
    }

    double vl = (linear_x + angular_z * BASE_WIDTH / 2.0); //meters/sec
    double vr = (linear_x - angular_z * BASE_WIDTH / 2.0);  

    desired_vel_mutex.lock();
    desired_vl = int(vl * TICKS_PER_METER); //ticks/sec
    desired_vr = int(vr * TICKS_PER_METER);  
    desired_vel_mutex.unlock();
  }
  planner_mutex.unlock();
}

double OdometryPublisher::normalize_angle(double angle) {
  double pi = 3.141592654;
  while (angle > pi) {
    angle -= 2.0 * pi;
  }
  while (angle < -pi) {
    angle += 2.0 * pi;
  }

  return angle;
}
