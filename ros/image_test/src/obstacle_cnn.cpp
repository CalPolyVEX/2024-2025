//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <ros/ros.h>
#include <ros/package.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/PointField.h>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"

#define _USE_MATH_DEFINES
#include <cmath>

using namespace cv;
using namespace std;
//using namespace tensorflow;

class ImageConverter {
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  image_transport::Publisher image_pub_;
  ros::Publisher point_pub;
  cv::Mat new_image;
  tensorflow::Tensor *input_tensor;
  float scan[48];
  int scan_counter = 0;
  
  tensorflow::Session* session; //tensorflow session
  tensorflow::GraphDef graph_def;

  public:

  void run_network(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImageConstPtr cv_ptr;
    tensorflow::Status status;

    try {
      cv_ptr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::BGR8);
    } catch (cv_bridge::Exception& e) {
      ROS_ERROR("cv_bridge exception: %s", e.what());
    }
    
    //resize image to 480x270
    cv::resize(cv_ptr->image, new_image, cv::Size(480,270), CV_INTER_LINEAR);

    auto input_tensor_mapped = input_tensor->tensor<float, 4>();

    //fill in tensor with image data
    for (int y = 0; y < 270; y++) {
        for (int x = 0; x < 480; x++) {
            Vec3b pixel = new_image.at<Vec3b>(y, x);

            input_tensor_mapped(0, y, x, 0) = ((float)pixel.val[0]); //R
            input_tensor_mapped(0, y, x, 1) = ((float)pixel.val[1]); //G
            input_tensor_mapped(0, y, x, 2) = ((float)pixel.val[2]); //B
        }
    }

    // The session will initialize the outputs
    std::vector<tensorflow::Tensor> outputs;

    // Run the session, evaluating our "c" operation from the graph
    /* status = session->Run({new_image}, &outputs); */
    string input = "input_1";
    pair <string,tensorflow::Tensor> p(input, *input_tensor);
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
    int col_counter = 5;
    
    for (int i=0; i < 48; i++) {
      x = output_c(i);
      x = x * 270.0;
      scan[i] = x;

      if (1==0) {
        circle(new_image, Point(col_counter, (int)x), 3, Scalar(0,0,255), -1);
      }

      col_counter += 10;
    }

    if (1==0) {
      //publish message
      sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
      image_pub_.publish(pub_msg);
    }

    publish_point_cloud();
  }

  void publish_point_cloud() {
    // Fill in new PointCloud2 message (2D image-like layout)
    sensor_msgs::PointCloud2 points_msg = sensor_msgs::PointCloud2();

    /* points_msg->header = disp_msg->header; */
    points_msg.header.stamp = ros::Time::now();
    points_msg.header.seq = scan_counter;
    points_msg.header.frame_id = "base_link";
    scan_counter++;

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
    float cur_x = 5;
    float z = 0;
    for (int u = 47; u >= 0 ; u--) {
        float t_x = (cur_x / 240.0);
        float t_y = (270.0-scan[u]) / 135.0;
        // x,y,z,rgba
        memcpy (&points_msg.data[offset + 0], &t_y, sizeof (float));
        memcpy (&points_msg.data[offset + 4], &t_x, sizeof (float));
        memcpy (&points_msg.data[offset + 8], &z, sizeof (float));

        //set the color
        unsigned int color = 0xff0000ff;
        memcpy (&points_msg.data[offset + 12], &color, sizeof (int));

        cur_x += 10;
        offset += 16;
    }

    point_pub.publish(points_msg);
  }

  ImageConverter() : it_(nh_) {
    //new_image = Mat(270, 480, CV_8UC(3));
    new_image = Mat(270, 480, CV_32FC(3));

    input_tensor = new tensorflow::Tensor(tensorflow::DT_FLOAT, 
        tensorflow::TensorShape({1, new_image.rows, new_image.cols, 3})); 

    float_t *p1 = input_tensor->flat<float_t>().data(); 

    image_transport::TransportHints hints("compressed");
    image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &ImageConverter::run_network, this, hints);
    /* image_sub_ = it_.subscribe("/zed/data_throttled_image", 1, &ImageConverter::run_network, this); */
    image_pub_ = it_.advertise("/image_converter/output_video", 1);
    point_pub = nh_.advertise<sensor_msgs::PointCloud2>("/test_point_cloud", 1);
  }

  int init_tensorflow() {
    // Initialize a tensorflow session
    tensorflow::SessionOptions options = tensorflow::SessionOptions();
    options.config.mutable_gpu_options()->set_allow_growth(true);
    tensorflow::Status status = tensorflow::NewSession(options, &session);

    if (!status.ok()) { //if there is an error
      std::cout << status.ToString() << "\n";
      return 1;
    }

    // Read in the protobuf graph we exported.
    status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), ros::package::getPath("image_test") + "/models/output_graph.pb", &graph_def);
    if (!status.ok()) {
      std::cout << status.ToString() << "\n";
      return 1;
    }

    status = session->Create(graph_def);
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
