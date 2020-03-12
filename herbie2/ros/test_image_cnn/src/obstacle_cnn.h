#ifndef OBSTACLE_CNN_H
#define OBSTACLE_CNN_H

#include <ros/ros.h>
#include <ros/package.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/video/tracking.hpp>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/PointField.h>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include <assert.h>

#define _USE_MATH_DEFINES
#include <cmath>

using namespace cv;
using namespace std;

#define NUM_OUTPUTS 128
#define IMG_HEIGHT 360
#define IMG_WIDTH 640

class ObstacleDetection {
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  image_transport::Publisher image_pub_;
  ros::Publisher point_pub;
  cv::Mat new_image;
  tensorflow::Tensor *input_tensor;
  float scan[NUM_OUTPUTS]; //index 0 is left, index 47 is right 
  KalmanFilter* kf[NUM_OUTPUTS];
  int scan_counter = 0;
  cv::Mat r_t_inv;
  
  tensorflow::Session* session; //tensorflow session
  tensorflow::GraphDef graph_def;
  string input = "input_1";
  int rows = IMG_HEIGHT;
  int cols = IMG_WIDTH;
  pair <string,tensorflow::Tensor> test_pair;
  vector <std::pair<string,tensorflow::Tensor>> test_v;
  float norm = 1.0000000 / 255.0000000;

  public:
    ObstacleDetection();
    void run_network(Mat* m); 
    int init_tensorflow();
};

#endif
