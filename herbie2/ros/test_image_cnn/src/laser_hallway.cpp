//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <ros/ros.h>
#include <ros/package.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/PointField.h>
#include <sensor_msgs/LaserScan.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <laser_geometry/laser_geometry.h>

#include <pcl/point_types.h>
#include <pcl/sample_consensus/ransac.h>
#include <pcl/sample_consensus/sac_model_line.h>
#include <pcl/point_cloud.h>
#include <pcl/common/io.h>

#define _USE_MATH_DEFINES
#include <cmath>

ros::NodeHandle *nh;

using namespace std;

class ScanProcessor {
  ros::Subscriber scan_sub;
  ros::Publisher point_pub;
  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  laser_geometry::LaserProjection projector_;
 
  public:

  void convert_scan(const sensor_msgs::LaserScan::ConstPtr& msg) {
    sensor_msgs::PointCloud cloud;
    projector_.projectLaser(*msg, cloud);

    // // initialize PointClouds
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud1 (new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr final1 (new pcl::PointCloud<pcl::PointXYZ>);

    // populate our PointCloud with points
    cloud1->width    = 100;
    cloud1->height   = 1;
    cloud1->is_dense = false;
    cloud1->points.resize (cloud1->width * cloud1->height);

    int i=0;
    for (auto it = begin (cloud.points); it != end (cloud.points); ++it) {
      float x = it->x;
      float y = it->y;

      float dist = sqrt(x*x + y*y); 
      if (dist < 3.0) { //distance is less than 3.0m
          cloud1->points[i].x = x;
          cloud1->points[i].y = y;
          cloud1->points[i].z = 0;
          i++;
      }
    }

    //run RANSAC
    std::vector<int> inliers;
    pcl::SampleConsensusModelLine<pcl::PointXYZ>::Ptr model_l(new pcl::SampleConsensusModelLine<pcl::PointXYZ> (cloud1));

    pcl::RandomSampleConsensus<pcl::PointXYZ> ransac (model_l);
    ransac.setDistanceThreshold (.01);
    ransac.computeModel();
    ransac.getInliers(inliers);

    // copies all inliers of the model computed to another PointCloud
    pcl::copyPointCloud<pcl::PointXYZ>(*cloud1, inliers, *final1);
    ROS_INFO("original size: %d", i);
    ROS_INFO("inliers size: %d", (int)inliers.size());

    publish_point_cloud(final1);
  }

  void publish_point_cloud(pcl::PointCloud<pcl::PointXYZ>::Ptr final1) {
    // Fill in new PointCloud2 message (2D image-like layout)
    sensor_msgs::PointCloud2 points_msg = sensor_msgs::PointCloud2();

    /* points_msg->header = disp_msg->header; */
    points_msg.header.stamp = ros::Time::now();
    points_msg.header.seq = 0;
    points_msg.header.frame_id = "zed_camera_center";

    points_msg.height = 1;
    points_msg.width  = final1->width;
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
    for (int u = 0; u < 48 ; u++) { //go through the 48 points
      float x = final1->points[u].x;
      float y = final1->points[u].y;
      float z = final1->points[u].z;

      // x,y,z
      memcpy (&points_msg.data[offset + 0], &x, sizeof (float));
      memcpy (&points_msg.data[offset + 4], &y, sizeof (float));
      memcpy (&points_msg.data[offset + 8], &z, sizeof (float));

      //set the color
      unsigned int color = 0xff0000ff;
      memcpy (&points_msg.data[offset + 12], &color, sizeof (int));

      offset += 16;
    }

    point_pub.publish(points_msg);
  }

  ScanProcessor() : tf_listener_(tf_buffer_) {
    scan_sub = nh->subscribe("/laser_scan_filtered", 1, &ScanProcessor::convert_scan, this);
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
