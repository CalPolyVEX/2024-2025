#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include "encoder_odom.h"
using namespace std;

extern ros::NodeHandle nh;
extern ros::Subscriber sub_;
extern ros::Publisher pub_, odom_req;

void OdometryPublisher::publish_odometry_message(void) {
  int tick_x = 0;
  int tick_y = 0;

  ros::Time current_time = ros::Time::now();
  double DistancePerCount = (3.14159265 * 0.13) / 2626; 
  double lengthBetweenTwoWheels = 0.25;

  //extract the wheel velocities from the tick signals count
  int deltaLeft = tick_x - _PreviousLeftEncoderCounts;
  int deltaRight = tick_y - _PreviousRightEncoderCounts;

  double omega_left = (deltaLeft * DistancePerCount) / (current_time - last_time).toSec();
  double omega_right = (deltaRight * DistancePerCount) / (current_time - last_time).toSec();

  double v_left = omega_left * 0.065; //radius
  double v_right = omega_right * 0.065;

  double vx = ((v_right + v_left) / 2)*10;
  double vy = 0;
  double vth = ((v_right - v_left)/lengthBetweenTwoWheels)*10;

  double dt = (current_time - last_time).toSec();
  double delta_x = (vx * cos(th)) * dt;
  double delta_y = (vx * sin(th)) * dt;
  double delta_th = vth * dt;

  x += delta_x;
  y += delta_y;
  th += delta_th;

  tf2::Quaternion odom_quat;
  odom_quat.setRPY(0,0,0);

  geometry_msgs::TransformStamped odom_trans;
  odom_trans.header.stamp = current_time;
  odom_trans.header.frame_id = "robot_odom";
  odom_trans.child_frame_id = "base_link";

  odom_trans.transform.translation.x = x;
  odom_trans.transform.translation.y = y;
  odom_trans.transform.translation.z = 0.0;
  //odom_trans.transform.rotation = odom_quat;

  //send the transform
  //odom_broadcaster.sendTransform(odom_trans);

  //Odometry message
  nav_msgs::Odometry odom;
  odom.header.stamp = current_time;
  odom.header.frame_id = "odom";

  //set the position
  odom.pose.pose.position.x = x;
  odom.pose.pose.position.y = y;
  odom.pose.pose.position.z = 0.0;
  //odom.pose.pose.orientation = odom_quat;

  //set the velocity
  odom.child_frame_id = "base_link";
  odom.twist.twist.linear.x = vx;
  odom.twist.twist.linear.y = vy;
  odom.twist.twist.angular.z = vth;

  //publish the message
  pub_.publish(odom);
  _PreviousLeftEncoderCounts = tick_x;
  _PreviousRightEncoderCounts = tick_y;

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

void OdometryPublisher::update(int enc_left, int enc_right, double* vel_x, double* vel_theta) {
  int left_ticks, right_ticks;
  double dist_left, dist_right, dist;
  double d_theta, r;
  ros::Time current_time;

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
  current_time = ros::Time::now();
  double d_time = (current_time - last_enc_time).toSec();
  last_enc_time = current_time;

  double current_left_vel = left_ticks / d_time; //ticks/second
  double current_right_vel = right_ticks / d_time;

  //if last_left_vel != 0 and last_right_vel != 0:

  //when computing the current tick velocity, filter the readings
  //from the encoders as the readings are noisy at slow speeds
  left_tick_vel =  .7*left_tick_vel + .3*(left_ticks / d_time); //should be in ticks/second
  right_tick_vel = .7*right_tick_vel + .3*(right_ticks / d_time);

  // TODO find better what to determine going straight,
  // this means slight deviation is accounted
  if (left_ticks == right_ticks) {
    d_theta = 0.0;
    cur_x += dist * cos(cur_theta);
    cur_y += dist * sin(cur_theta);
  } else {
    d_theta = (dist_right - dist_left) / BASE_WIDTH;
    r = dist / d_theta;
    cur_x += r * (sin(d_theta + cur_theta) -
        sin(cur_theta));
    cur_y -= r * (cos(d_theta + cur_theta) -
        cos(cur_theta));
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

void OdometryPublisher::update_publish_cb(const std_msgs::Int32MultiArray::ConstPtr& enc_msg) {
  //this callback is called when a new encoder message is
  //received from the Arduino
  int enc_left, enc_right;

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
    //vel_x, vel_theta = update(enc_left, enc_right);
    //publish_odometry_message(cur_x, cur_y, cur_theta, vel_x, vel_theta)
  }


            /*
    def update( enc_left, enc_right):
        #take the encoder counts and update the number of ticks traveled
        left_ticks = enc_left - last_enc_left
        right_ticks = enc_right - last_enc_right
        last_enc_left = enc_left
        last_enc_right = enc_right

        dist_left = left_ticks / TICKS_PER_METER
        dist_right = right_ticks / TICKS_PER_METER
        dist = (dist_right + dist_left) / 2.0

        current_time = rospy.Time.now()
        d_time = (current_time - last_enc_time).to_sec()
        last_enc_time = current_time

        last_left_vel = left_ticks / d_time #ticks/second
        last_right_vel = right_ticks / d_time
        #if last_left_vel != 0 and last_right_vel != 0:

        #when computing the current tick velocity, filter the readings
        #from the encoders as the readings are noisy at slow speeds
        left_tick_vel =  .7*left_tick_vel + .3*(left_ticks / d_time) #should be in ticks/second
        right_tick_vel = .7*right_tick_vel + .3*(right_ticks / d_time)

        # TODO find better what to determine going straight,
        # this means slight deviation is accounted
        if left_ticks == right_ticks:
            d_theta = 0.0
            cur_x += dist * cos(cur_theta)
            cur_y += dist * sin(cur_theta)
        else:
            d_theta = (dist_right - dist_left) / BASE_WIDTH
            r = dist / d_theta
            cur_x += r * (sin(d_theta + cur_theta) -
                               sin(cur_theta))
            cur_y -= r * (cos(d_theta + cur_theta) -
                               cos(cur_theta))
            cur_theta = normalize_angle(cur_theta - d_theta)

        if abs(d_time) < 0.000001:
            vel_x = 0.0
            vel_theta = 0.0
        else:
            vel_x = dist / d_time
            vel_theta = d_theta / d_time

        return vel_x, vel_theta
*/
}
