//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <ros/ros.h>
#include <ros/package.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/video/tracking.hpp>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/PointField.h>
#include <sensor_msgs/LaserScan.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>

#define _USE_MATH_DEFINES
#include <cmath>

ros::NodeHandle *nh;

using namespace cv;
using namespace std;

class ScanProcessor {
  ros::Subscriber scan_sub;
  ros::Publisher point_pub;
  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
 
  public:

  void convert_scan(const sensor_msgs::LaserScan& msg) {
  }

  void publish_point_cloud() {
    // Fill in new PointCloud2 message (2D image-like layout)
    sensor_msgs::PointCloud2 points_msg = sensor_msgs::PointCloud2();

    /* points_msg->header = disp_msg->header; */
    points_msg.header.stamp = ros::Time::now();
    points_msg.header.seq = 0;
    points_msg.header.frame_id = "zed_camera_center";

    points_msg.height = 1;
    points_msg.width  = 48;
    points_msg.fields.resize (4);
    points_msg.fields[0].name = "x";
    points_msg.fields[0].offset = 0;
    points_msg.fields[0].count = 1;
    points_msg.fields[0].datatype = sensor_msgs::PointField::FLOAT32;
    points_msg.fields[1].name = "y";
    points_msg.fields[1].offset = 4;
    points_msg.fields[1].count = 1;
    points_msg.fields[1].datatype = sensor_msgs::PointField::FLOAT32;
    points_msg.fields[2].name = "z";
    points_msg.fields[2].offset = 8;
    points_msg.fields[2].count = 1;
    points_msg.fields[2].datatype = sensor_msgs::PointField::FLOAT32;
    points_msg.fields[3].name = "rgb";
    points_msg.fields[3].offset = 12;
    points_msg.fields[3].count = 1;
    points_msg.fields[3].datatype = sensor_msgs::PointField::UINT32;
    //points_msg->is_bigendian = false; ???
    static const int STEP = 16;
    points_msg.point_step = STEP;
    points_msg.row_step = points_msg.point_step * points_msg.width;
    points_msg.data.resize (points_msg.row_step * points_msg.height);
    points_msg.is_dense = true; // there are no invalid points

    int offset = 0;
    float cur_x = 5; //the starting x coordinate
    float z = .02; //the ground is a plane so the Z-coordinate is 0
    for (int u = 0; u < 48 ; u++) { //go through the 48 points
      float img_x = cur_x;
      float img_y = 0;

      double c_x,c_y;
      transform_point(img_x, img_y, &c_x, &c_y);
      float temp_x, temp_y;
      temp_x = -c_x;
      temp_y = c_y;

      // x,y,z
      memcpy (&points_msg.data[offset + 0], &temp_y, sizeof (float));
      memcpy (&points_msg.data[offset + 4], &temp_x, sizeof (float));
      memcpy (&points_msg.data[offset + 8], &z, sizeof (float));

      //set the color
      unsigned int color = 0xff0000ff;
      memcpy (&points_msg.data[offset + 12], &color, sizeof (int));

      cur_x += 10;
      offset += 16;
    }

    point_pub.publish(points_msg);
  }

  void transform_point(double x, double y, double* x_object, double* y_object) {
  }

  ScanProcessor() : tf_listener_(tf_buffer_) {

    scan_sub = nh->subscribe("/see3cam_cu20/image_raw", 1, &ScanProcessor::convert_scan, this);
    point_pub = nh->advertise<sensor_msgs::PointCloud2>("/test_point_cloud", 1);

  }

};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "laserscan_node");
  ros::NodeHandle n;
  nh = &n;

  ScanProcessor scan_processor;
  ros::spin();
}
