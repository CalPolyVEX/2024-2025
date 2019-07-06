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
//using namespace tensorflow;

class ImageConverter
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  image_transport::Publisher image_pub_;
  cv::Mat new_image;

  tensorflow::Session* session; //tensorflow session
  tensorflow::GraphDef graph_def;

  public:

  void run_network(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImagePtr cv_ptr;
    tensorflow::Status status;
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

    //fill in tensor with image data
    int depth = 3;
    tensorflow::Tensor input_tensor(tensorflow::DT_FLOAT,
        tensorflow::TensorShape({1, new_image.rows, new_image.cols, depth}));

    auto input_tensor_mapped = input_tensor.tensor<float, 4>();

    for (int y = 0; y < new_image.rows; y++) {
      for (int x = 0; x < new_image.cols; x++) {
        Vec3b pixel = new_image.at<Vec3b>(y, x);

        input_tensor_mapped(0, y, x, 0) = ((float)pixel.val[2])/255.0; //R
        input_tensor_mapped(0, y, x, 1) = ((float)pixel.val[1])/255.0; //G
        input_tensor_mapped(0, y, x, 2) = ((float)pixel.val[0])/255.0; //B
      }
    }

    // The session will initialize the outputs
    std::vector<tensorflow::Tensor> outputs;

    // Run the session, evaluating our "c" operation from the graph
    /* status = session->Run({new_image}, &outputs); */
    string input = "input_1";
    pair <string,tensorflow::Tensor> p(input, input_tensor);
    vector <std::pair<string,tensorflow::Tensor>> v{p};

    status = session->Run(v, {"k2tfout_0"}, {}, &outputs);
    if (!status.ok()) {
      std::cout << status.ToString() << "\n";
    }

    //print out the dimension of the tensor
    //cout << outputs[0].shape().dim_size(0);

    // Grab the first output (we only evaluated one graph node: "c")
    // and convert the node to a scalar representation.
    auto output_c = outputs[0].tensor<float,1>();
    float x;
    int col_counter = 0;
    
    for (int i=0; i < 48; i++) {
      x = output_c(i);
      x = x * 270.0;

      circle(new_image, Point(col_counter, (int)x), 3, Scalar(0,0,255), -1);
      col_counter += 10;
    }

    //publish message
    sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
    image_pub_.publish(pub_msg);
  }

  ImageConverter() :
      it_(nh_)
    {
      image_transport::TransportHints hints("compressed");
      image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &ImageConverter::run_network, this, hints);
      /* image_sub_ = it_.subscribe("/zed/data_throttled_image", 1, &ImageConverter::run_network, this); */
      image_pub_ = it_.advertise("/image_converter/output_video", 1);
      new_image = Mat(270, 480, CV_8UC(3));
    }

  int init_tensorflow() {
    // Initialize a tensorflow session
    tensorflow::SessionOptions options = tensorflow::SessionOptions();
    options.config.mutable_gpu_options()->set_allow_growth(true);
    tensorflow::Status status = tensorflow::NewSession(options, &session);
    if (!status.ok()) {
      std::cout << status.ToString() << "\n";
      return 1;
    }

    // Read in the protobuf graph we exported.
    status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), ros::package::getPath("tensorflow_ros_test") + "/models/output_graph.pb", &graph_def);
    if (!status.ok()) {
      std::cout << status.ToString() << "\n";
      return 1;
    }

    status = session->Create(graph_def);
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
