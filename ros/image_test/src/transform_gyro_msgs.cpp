//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <tensorflow/core/public/session.h>

#define _USE_MATH_DEFINES
#include <cmath>

using namespace cv;
using namespace std;

class ImageConverter
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  image_transport::Publisher image_pub_;
  cv::Mat new_image;

  void imageCb(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImagePtr cv_ptr;
    
    try
    {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }

    //resize image to 480x270
    cv::resize(cv_ptr->image, new_image, cv::Size(480,270), CV_INTER_LINEAR);
    
    // Draw an example circle on the video stream
    /* if (cv_ptr->image.rows > 60 && cv_ptr->image.cols > 60) */
    /*   cv::circle(cv_ptr->image, cv::Point(50, 50), 10, CV_RGB(255,0,0)); */

    // Output modified video stream
    //image_pub_.publish(cv_ptr->toImageMsg());
    
    sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
    image_pub_.publish(pub_msg);
  }

  public:

  ImageConverter() :
      it_(nh_)
    {
      /* image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &ImageConverter::imageCb, this); */
      image_sub_ = it_.subscribe("/zed/data_throttled_image", 1, &ImageConverter::imageCb, this);
      image_pub_ = it_.advertise("/image_converter/output_video", 1);
      new_image = Mat(270, 480, CV_8UC3, Scalar(0,0,0));
    }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "gyro_pub");
  ImageConverter gyro_pub;
  ros::spin();
}
