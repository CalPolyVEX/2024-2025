#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <sys/time.h>
#include "ros/ros.h"
#include <ros/console.h>
#include <cstdlib>
#include "unistd.h"
#include "v4l2_helper.h"
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <signal.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Empty.h>

//NVIDIA includes
#include <boost/filesystem.hpp>
#include <cuda_runtime_api.h>
#include "NvOnnxParser.h"
#include "NvInfer.h"
 
#ifdef ARM_CPU
  extern "C" void asm_foo(unsigned char* in, unsigned char* out);
#endif

using namespace std;
using namespace cv;

class CameraReader {
  unsigned int width, height;
  //const char* default_videodev = "/dev/video0";
  const char *videodev;
  unsigned int start, end, fps = 0;
  unsigned char* ptr_cam_frame;
  int bytes_used;
  ros::NodeHandle nh_;
  ros::NodeHandle* n;
  image_transport::ImageTransport it_;
  image_transport::Publisher image_pub_;
  image_transport::Subscriber image_sub_;
  ros::Subscriber image_debug_toggle_;
  ros::Publisher ground_pub;

  /*
   * Re-using the frame matrix(ces) instead of creating new ones (i.e., declaring 'Mat frame'
   * (and cuda::GpuMat gpu_frame) outside the 'while (1)' loop instead of declaring it
   * within the loop) improves the performance for higher resolutions.
   */
  Mat uyvy_frame, bgr_frame, bgr_frame_360;

  ////////////////////////////////////////////////
  //variables for inference
  struct nn_context {
    nvinfer1::IRuntime *runtime;
    nvinfer1::ICudaEngine *engine;
    nvinfer1::IExecutionContext *context;
    void **mInputCPU;
    void *buffers[5];
    cudaStream_t stream;
    int inputIndex;
    int outputIndex;
  };

  struct nn_context nn1;
  struct nn_context nn2;
  int network_counter = 0;

  public:
  int init(char* videodev, int width, int height, ros::NodeHandle* nh, bool simulate);
  void nhwc_to_nchw(unsigned char* src, float* dest, int nn_height, int nn_width);
  void frame_loop();
  void simulate_callback(const sensor_msgs::ImageConstPtr& msg);
  int close_camera();

  //neural network functions
  static void buildEngine(char* s, int dla);
  //void initInference(char* s);
  void initInference(char* s);
  //void inference();
  void inference(struct nn_context* nn);
  void endInference();
  void image_pub_toggle_cb(const std_msgs::Empty::ConstPtr&);

  CameraReader() : it_(nh_) {
    image_transport::TransportHints hints("compressed");
    image_pub_ = it_.advertise("/see3cam_cu20/image_raw1", 1);
    image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &CameraReader::simulate_callback, this, hints);
  }

};

#endif