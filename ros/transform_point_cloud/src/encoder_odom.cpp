#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <cmath>
#include "encoder_odom.h"

using namespace std;

extern ros::NodeHandle *nh;
extern ros::Subscriber sub_;
extern ros::Publisher pub_, odom_req;

void OdometryPublisher::publish_odometry_message(double vx, double vth) {
  //publish a new odometry message
  tf2::Quaternion odom_quat;
  odom_quat.setRPY(0,0,cur_theta);
  ros::Time current_time = ros::Time::now();

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

  //set the position
  odom.pose.pose.position.x = cur_x;
  odom.pose.pose.position.y = cur_y;
  odom.pose.pose.position.z = 0.0;
  //odom.pose.pose.orientation = odom_quat;

  odom.pose.covariance[0] = 0.01;
  odom.pose.covariance[7] = 0.01;
  odom.pose.covariance[14] = 99999;
  odom.pose.covariance[21] = 99999;
  odom.pose.covariance[28] = 99999;
  odom.pose.covariance[35] = 0.01;

  //set the velocity
  odom.child_frame_id = "roboclaw_center";
  odom.twist.twist.linear.x = vx;
  odom.twist.twist.linear.y = 0;
  odom.twist.twist.angular.z = vth;

  //set the twist covariance
  odom.twist.covariance[0] = 0.01;
  odom.twist.covariance[7] = 0.01;
  odom.twist.covariance[14] = 99999;
  odom.twist.covariance[21] = 99999;
  odom.twist.covariance[28] = 99999;
  odom.twist.covariance[35] = 0.01;

  //send the message
  pub_.publish(odom);

  last_time = current_time;
}

double OdometryPublisher::normalize_angle(double angle) {
  double pi = 3.14159;
  while (angle > pi) {
    angle -= 2.0 * pi;
  }
  while (angle < -pi) {
    angle += 2.0 * pi;
  }

  return angle;
}

void OdometryPublisher::update_odometry(int enc_left, int enc_right, double* vel_x, double* vel_theta) {
  int left_ticks, right_ticks;
  double dist_left, dist_right, dist;
  double d_theta, r;
  ros::Time current_time = ros::Time::now();

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
  mutex.lock();
  left_tick_vel =  .7*left_tick_vel + .3*(current_left_vel); //should be in ticks/second
  right_tick_vel = .7*right_tick_vel + .3*(current_right_vel);
  mutex.unlock();

  // TODO find better what to determine going straight,
  // this means slight deviation is accounted
  if (left_ticks == right_ticks) {
    d_theta = 0.0;
    cur_x += dist * cos(cur_theta);
    cur_y += dist * sin(cur_theta);
  } else {
    d_theta = (dist_right - dist_left) / BASE_WIDTH;
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
    *vel_theta = d_theta / d_time;
  }

  return;
}

void OdometryPublisher::encoder_message_callback(const std_msgs::Int32MultiArray::ConstPtr& enc_msg) {
  //this callback is called when a new encoder message is
  //received from the Arduino
  int enc_left, enc_right;
  double vel_x, vel_theta;

  enc_left = enc_msg->data[0];
  enc_right = enc_msg->data[1];

  //2106 per 0.1 seconds is max speed, error in the 16th bit is 32768
  //todo lets find a better way to deal with this error
  if (abs(enc_left - last_enc_left) > 20000) {
    ROS_INFO("Ignoring left encoder jump: cur %d, last %d",  enc_left, last_enc_left);
  } else if (abs(enc_right - last_enc_right) > 20000) {
    ROS_INFO("Ignoring right encoder jump: cur %d, last %d", enc_right, last_enc_right);
  } else {
    //call the update function
    update_odometry(enc_left, enc_right, &vel_x, &vel_theta);
    publish_odometry_message(vel_x, vel_theta);
  }
}

void OdometryPublisher::compute_pid(double left_desired, double left_actual, double right_desired, double right_actual, double* left_set_value, double* right_set_value) {
  double kp = 5;
  double ki = .37;
  double kd = .02;

  double left_error = left_desired - left_actual;
  double right_error = right_desired - right_actual;
  double left_error_diff = last_left_error - left_error;
  double right_error_diff = last_right_error - right_error;

  //compute integral term
  double left_sum = 0;
  double right_sum = 0;

  for (int i=0; i++; i<INTEGRAL_ARRAY_SIZE) {
    left_sum += left_integral[i];
    right_sum += right_integral[i];
  }

  if (left_sum > 5000 || left_sum < -5000) {
    left_sum = 5000;
  }

  if (right_sum > 5000 || right_sum < -5000) {
    right_sum = 5000;
  }

  left_counter = (left_counter + 1) % INTEGRAL_ARRAY_SIZE;
  right_counter = (right_counter + 1) % INTEGRAL_ARRAY_SIZE;
  left_integral[left_counter] = left_error;
  right_integral[right_counter] = right_error;

  *left_set_value = (kp * left_error) + (ki * left_sum) - (kd * left_error_diff);
  *right_set_value = (kp * right_error) + (ki * right_sum) - (kd * right_error_diff);

  last_left_error = left_error;
  last_right_error = right_error;

  if (abs(*left_set_value) > 7000) {
    *left_set_value = 7000;
  }
  if (abs(*right_set_value) > 7000) {
    *right_set_value = 7000;
  }
}

void OdometryPublisher::cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist) {
  //with self.lock:
  last_set_speed_time = ros::Time::now();

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

  double vr = (linear_x - angular_z * BASE_WIDTH / 2.0);  // m/s
  double vl = (linear_x + angular_z * BASE_WIDTH / 2.0);

  int vr_ticks = int(vr * TICKS_PER_METER);  // ticks/s
  int vl_ticks = int(vl * TICKS_PER_METER);

    /*
            #publish the wheel speeds to /motors/commanded_speeds
            v_wheels= Wheels_speeds()
            #v_wheels.wheel1=vl
            #v_wheels.wheel2=vr
            */
  double pid_speed_l, pid_speed_r;
  double ltv, rtv;

  mutex.lock();
  ltv = left_tick_vel;
  rtv = right_tick_vel;
  mutex.unlock();

  compute_pid(vl_ticks, ltv, vr_ticks, rtv, &pid_speed_l, &pid_speed_r);
  //v_wheels.wheel1=pid_speed_l;
  //v_wheels.wheel2=pid_speed_r;
  //self.wheels_speeds_pub.publish(v_wheels)

  ROS_INFO("desired vl_ticks: %d desired vr_ticks: %d", vl_ticks, vr_ticks);
  ROS_INFO("current left vel: %f current right vel: %f", \
            ltv, rtv);
  ROS_INFO("left error: %f right error: %f", \
            vl_ticks - ltv, \
            vr_ticks - rtv);

  int left_pid_output = int(pid_speed_l);
  int right_pid_output = int(pid_speed_r);
  ROS_INFO("left pid output: %d right pid output: %d", left_pid_output, right_pid_output);

  setmotor(0,left_pid_output);
  setmotor(1,right_pid_output);

  /*
            try:
                #Send motor commands to the Roboclaw
                roboclaw.DutyM1(self.address, -left_pid_output)
                roboclaw.DutyM2(self.address, -right_pid_output)
            except OSError as e:
                rospy.logwarn("SpeedM1M2 OSError: %d", e.errno)
                rospy.logdebug(e)
                */
}
