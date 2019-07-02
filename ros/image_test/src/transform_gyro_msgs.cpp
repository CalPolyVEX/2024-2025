//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <ros/ros.h>
#include <ros/package.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"

#define _USE_MATH_DEFINES
#include <cmath>

using namespace cv;
using namespace std;
using namespace tensorflow;

class ImageConverter
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  image_transport::Publisher image_pub_;
  cv::Mat new_image;

  Session* session; //tensorflow session
  GraphDef graph_def;

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
    
    //publish message
    sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
    image_pub_.publish(pub_msg);
  }


  public:

  void run_network(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImagePtr cv_ptr;
    Status status;
    // Setup inputs and outputs:

    try
    {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
    }

    //resize image to 480x270
    cv::resize(cv_ptr->image, new_image, cv::Size(480,270), CV_INTER_LINEAR);

    // Our graph doesn't require any inputs, since it specifies default values,
    // but we'll change an input to demonstrate.
    Tensor input_tensor(DT_UINT8, TensorShape({1, 270, 480, 3}));
    // get pointer to memory for that Tensor
    unsigned char* ptr = input_tensor.flat<unsigned char>().data();
    cv::Mat tensor_image(270, 480, CV_8UC3, ptr);
    tensor_image.convertTo(new_image, CV_8UC3);  

    // The session will initialize the outputs
    std::vector<tensorflow::Tensor> outputs;

    // Run the session, evaluating our "c" operation from the graph
    status = session->Run(new_image, {"k2tfout_"}, {}, &outputs);
    if (!status.ok()) {
      std::cout << status.ToString() << "\n";
    }

    /* cout << outputs.size(); */
    // Grab the first output (we only evaluated one graph node: "c")
    // and convert the node to a scalar representation.
    /* auto output_c = outputs[0].scalar<float>(); */
    //cout << output_c;
  }

  ImageConverter() :
      it_(nh_)
    {
      /* image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &ImageConverter::imageCb, this); */
      image_sub_ = it_.subscribe("/zed/data_throttled_image", 1, &ImageConverter::run_network, this);
      image_pub_ = it_.advertise("/image_converter/output_video", 1);
      new_image = Mat(270, 480, CV_8UC3);
    }

  int init_tensorflow() {
    // Initialize a tensorflow session
    tensorflow::SessionOptions options = SessionOptions();
    options.config.mutable_gpu_options()->set_allow_growth(true);
    Status status = NewSession(options, &session);
    if (!status.ok()) {
      std::cout << status.ToString() << "\n";
      return 1;
    }

    // Read in the protobuf graph we exported.
    status = ReadBinaryProto(Env::Default(), ros::package::getPath("tensorflow_ros_test") + "/models/output_graph.pb", &graph_def);
    if (!status.ok()) {
      std::cout << status.ToString() << "\n";
      return 1;
    }

    /* int node_count = graph_def.node_size(); */
    /* for (int i=0; i< node_count; i++) { */
    /*   auto n = graph_def.node(i); */
    /*   cout << "Names: " << n.name() << endl; */
    /* } */

    return 0;
  }

};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "gyro_pub");
  ImageConverter gyro_pub;
  gyro_pub.init_tensorflow();
  ros::spin();
}
