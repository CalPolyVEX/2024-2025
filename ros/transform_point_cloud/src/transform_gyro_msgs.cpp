//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/Vector3.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <sensor_msgs/Imu.h>

#define _USE_MATH_DEFINES
#include <cmath>

class GyroPublisher
{
  ros::NodeHandle nh_;
  ros::Subscriber sub_;
  ros::Publisher pub_;
  ros::Subscriber sub_a;
  ros::Publisher pub_a;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  sensor_msgs::Imu imu_g;
  sensor_msgs::Imu imu_a;
  int gyro_counter = 0;

  void gyroCallBack(const sensor_msgs::Imu::ConstPtr& msg) {
      /* ros::Time current_time = ros::Time::now(); */
      int i;

      //publish a new message every other incoming gyro message
      gyro_counter++;
      if ((gyro_counter & 1) == 0) {
          return;
      }

      //Imu message
      imu_g.header.seq = msg->header.seq;
      imu_g.header.stamp = msg->header.stamp;
      imu_g.header.frame_id = "t265_link";

      //set the orientation
      imu_g.orientation.x = msg->orientation.x;
      imu_g.orientation.y = msg->orientation.y;
      imu_g.orientation.z = msg->orientation.z;
      imu_g.orientation.w = msg->orientation.w;
      for (i=0;i<9;i++) {
          imu_g.orientation_covariance[i] = msg->orientation_covariance[i];
      }
      
      //set the angular velocity - rotate the axes to be correct for the
      //robot_localization package
      imu_g.angular_velocity.x = msg->angular_velocity.z;
      imu_g.angular_velocity.y = msg->angular_velocity.x;
      imu_g.angular_velocity.z = msg->angular_velocity.y;
      for (i=0;i<9;i++) {
          imu_g.angular_velocity_covariance[i] = msg->angular_velocity_covariance[i];
      }
      
      //set the linear acceleration
      imu_g.linear_acceleration.x = msg->linear_acceleration.x;
      imu_g.linear_acceleration.y = msg->linear_acceleration.y;
      imu_g.linear_acceleration.z = msg->linear_acceleration.z;
      for (i=0;i<9;i++) {
          imu_g.linear_acceleration_covariance[i] = msg->linear_acceleration_covariance[i];
      }

      //publish the message
      pub_.publish(imu_g);
  }

  void accelCallBack(const sensor_msgs::Imu::ConstPtr& msg) {
      /* ros::Time current_time = ros::Time::now(); */
      int i;

      //Imu message
      imu_a.header.seq = msg->header.seq;
      imu_a.header.stamp = msg->header.stamp;
      imu_a.header.frame_id = "t265_link";

      //Roll & Pitch Equations
      double fXg, fYg, fZg;
      double roll, pitch;
      fXg = msg->linear_acceleration.x;
      fYg = msg->linear_acceleration.z;
      fZg = msg->linear_acceleration.y;

      roll  = (atan2(-fYg, fZg)*180.0)/M_PI;
      pitch = (atan2(fXg, sqrt(fYg*fYg + fZg*fZg))*180.0)/M_PI;//set the orientation

      imu_a.orientation.x = roll;
      imu_a.orientation.y = pitch;

      /* imu_a.orientation.x = 0; */
      /* imu_a.orientation.y = 0; */
      imu_a.orientation.z = 0;
      imu_a.orientation.w = 0;
      for (i=0;i<9;i++) {
          imu_a.orientation_covariance[i] = msg->orientation_covariance[i];
      }
      
      //set the angular velocity
      imu_a.angular_velocity.x = 0;
      imu_a.angular_velocity.y = 0;
      imu_a.angular_velocity.z = 0;
      for (i=0;i<9;i++) {
          imu_a.angular_velocity_covariance[i] = msg->angular_velocity_covariance[i];
      }
      
      //set the linear acceleration - rotate the axes to be correct for the
      //robot_localization package
      imu_a.linear_acceleration.x = msg->linear_acceleration.z;
      imu_a.linear_acceleration.y = msg->linear_acceleration.x;
      imu_a.linear_acceleration.z = msg->linear_acceleration.y;
      for (i=0;i<9;i++) {
          imu_a.linear_acceleration_covariance[i] = msg->linear_acceleration_covariance[i];
      }

      //publish the message
      pub_a.publish(imu_a);
  }

    public:

  GyroPublisher() :
      tf_listener_(tf_buffer_)
    {
        pub_ = nh_.advertise<sensor_msgs::Imu>("/t265_gyro_data", 3);
        sub_ = nh_.subscribe("/camera/gyro/sample", 1, &GyroPublisher::gyroCallBack, this);
        pub_a = nh_.advertise<sensor_msgs::Imu>("/t265_accel_data", 3);
        sub_a = nh_.subscribe("/camera/accel/sample", 1, &GyroPublisher::accelCallBack, this);
    }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "gyro_pub");
  GyroPublisher gyro_pub;
  ros::spin();
}
