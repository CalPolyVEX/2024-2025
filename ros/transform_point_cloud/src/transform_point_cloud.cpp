/* Copyright 2018 Lucas Walter

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <geometry_msgs/TransformStamped.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <transform_point_cloud/LookupTransformConfig.h>

class TransformPointCloud
{
  ros::NodeHandle nh_;
  ros::Subscriber sub_;
  ros::Publisher pub_;
  std::string target_frame_;
  double timeout_;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  transform_point_cloud::LookupTransformConfig config_;

  void pointCloudCallback(const sensor_msgs::PointCloud2::ConstPtr& msg)
  {
    // ROS_INFO_STREAM(msg->header.frame_id << " " << target_frame_);
    ROS_DEBUG_STREAM("input:");

    geometry_msgs::TransformStamped transform;
    try
    {
      // (target_frame, pc frame) preserves the world coordinates of the point cloud but shifts
      // the parent to target_frame_
      // (pc_frame, target_frame) shifts the point cloud to be relative to the target_frame
      // by the same amount it used to be relative to msg->header.frame_id,
      // but the frame will still be msg->header.frame_id when done.
      const std::string target_frame = "zed_camera_center"; 
      const std::string source_frame = "zed_camera_center";
      transform = tf_buffer_.lookupTransform(
          target_frame,
          source_frame,
          ros::Time(0));
      sensor_msgs::PointCloud2 cloud_out;
      tf2::doTransform(*msg, cloud_out, transform);

      ROS_DEBUG_STREAM("output:");
      pub_.publish(cloud_out);
    }
    catch (tf2::TransformException& ex)
    {
      ROS_WARN("%s", ex.what());
      return;
    }
  }

public:
  TransformPointCloud() :
    // nh_("~"),
    tf_listener_(tf_buffer_)
  {
    pub_ = nh_.advertise<sensor_msgs::PointCloud2>("/zed/point_cloud_transformed", 3);
    sub_ = nh_.subscribe("/voxel_grid/output", 1, &TransformPointCloud::pointCloudCallback, this);
  }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "transform_point_cloud");
  TransformPointCloud transform_point_cloud;
  ros::spin();
}
