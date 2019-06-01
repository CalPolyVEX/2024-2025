//6/1/19 This is a currently unused file to publish odometry when 
//each encoder message arrives
//
#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Int32MultiArray.h>

class OdometryPublisher
{
  ros::NodeHandle nh_;
  ros::Subscriber sub_;
  ros::Publisher pub_;
  int _PreviousLeftEncoderCounts;
  int _PreviousRightEncoderCounts;
  ros::Time last_time;
  double x,y,th;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  void odometryCallBack(const std_msgs::Int32MultiArray::ConstPtr& msg) {
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

    public:

  OdometryPublisher() :
      tf_listener_(tf_buffer_)
    {
        pub_ = nh_.advertise<nav_msgs::Odometry>("/wheel_odometry", 3);
        sub_ = nh_.subscribe("/voxel_grid/output", 1, &OdometryPublisher::odometryCallBack, this);
    }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "odom_pub");
  OdometryPublisher odom_pub;
  ros::spin();
}
