//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/Vector3.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <sensor_msgs/Imu.h>

class GyroPublisher
{
  ros::NodeHandle nh_;
  ros::Subscriber sub_;
  ros::Publisher pub_;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  sensor_msgs::Imu imu;

  void gyroCallBack(const sensor_msgs::Imu::ConstPtr& msg) {
      /* ros::Time current_time = ros::Time::now(); */
      int i;

      //Imu message
      imu.header.seq = msg->header.seq;
      imu.header.stamp = msg->header.stamp;
      imu.header.frame_id = "t265_link";

      //set the orientation
      imu.orientation.x = msg->orientation.x;
      imu.orientation.y = msg->orientation.y;
      imu.orientation.z = msg->orientation.z;
      imu.orientation.w = msg->orientation.w;
      for (i=0;i<9;i++) {
          imu.orientation_covariance[i] = msg->orientation_covariance[i];
      }
      
      //set the angular velocity
      imu.angular_velocity.x = msg->angular_velocity.x;
      imu.angular_velocity.y = msg->angular_velocity.y;
      imu.angular_velocity.z = msg->angular_velocity.z;
      for (i=0;i<9;i++) {
          imu.angular_velocity_covariance[i] = msg->angular_velocity_covariance[i];
      }
      
      //set the linear acceleration
      imu.linear_acceleration.x = msg->linear_acceleration.x;
      imu.linear_acceleration.y = msg->linear_acceleration.y;
      imu.linear_acceleration.z = msg->linear_acceleration.z;
      for (i=0;i<9;i++) {
          imu.linear_acceleration_covariance[i] = msg->linear_acceleration_covariance[i];
      }

      //publish the message

      pub_.publish(imu);
  }

    public:

  GyroPublisher() :
      tf_listener_(tf_buffer_)
    {
        pub_ = nh_.advertise<sensor_msgs::Imu>("/new_gyro_data", 3);
        sub_ = nh_.subscribe("/camera/gyro/sample", 1, &GyroPublisher::gyroCallBack, this);
    }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "gyro_pub");
  GyroPublisher gyro_pub;
  ros::spin();
}
