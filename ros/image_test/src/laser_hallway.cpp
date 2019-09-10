//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <ros/ros.h>
#include <ros/package.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>
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
    ROS_INFO("original size: %d", i);
    ROS_INFO("inliers size: %d", (int)inliers.size());
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
