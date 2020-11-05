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

  void run_network(const sensor_msgs::ImageConstPtr& msg) {
    cv_bridge::CvImageConstPtr cv_ptr;
    tensorflow::Status status;

    try {
      cv_ptr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::BGR8);
    } catch (cv_bridge::Exception& e) {
      ROS_ERROR("cv_bridge exception: %s", e.what());
    }
    
    float* imgTensorFlat = input_tensor->flat<float>().data(); 
    /* imgTensorFlat = (test_v.data())->second.flat<float>().data(); */
    unsigned char* p1;

    int test = 0;
    if (test == 1) {
      //resize image to IMG_HEIGHT x IMG_WIDTH
      cv::resize(cv_ptr->image, new_image, cv::Size(IMG_WIDTH,IMG_HEIGHT), CV_INTER_LINEAR);

      //fill in tensor with image data
      /* assert(cv_ptr->image.cols == IMG_WIDTH); */
      /* assert(cv_ptr->image.rows == IMG_HEIGHT); */
      /* assert(cv_ptr->image.isContinuous() == true); */
      for(int i = 0; i < IMG_HEIGHT; i++) {
        p1 = (unsigned char*) cv_ptr->image.ptr<unsigned char>(i);
        for (int j = 0; j < (3*IMG_WIDTH); j=j+3) {
          unsigned char b = p1[j] ;
          unsigned char g = p1[j + 1];
          unsigned char r = p1[j + 2];

          *imgTensorFlat = ((float) r) * norm;
          imgTensorFlat++;
          *imgTensorFlat = ((float) g) * norm;
          imgTensorFlat++;
          *imgTensorFlat = ((float) b) * norm;
          imgTensorFlat++;
        }
      }
    } else {
      //resize image to IMG_HEIGHT x IMG_WIDTH
      cv::resize(cv_ptr->image, new_image, cv::Size(IMG_WIDTH,IMG_HEIGHT), CV_INTER_LINEAR);

      //assert(new_image.isContinuous() == true);
      for (int y = 0; y < IMG_HEIGHT; y++) {
        for (int x = 0; x < IMG_WIDTH; x++) {
          Vec3b pixel = new_image.at<Vec3b>(y, x);

          *imgTensorFlat = ((float) pixel.val[2]) * norm;
          imgTensorFlat++;
          *imgTensorFlat = ((float) pixel.val[1]) * norm;
          imgTensorFlat++;
          *imgTensorFlat = ((float) pixel.val[0]) * norm;
          imgTensorFlat++;
        }
      }
    }

    // The session will initialize the outputs
    std::vector<tensorflow::Tensor> outputs;

    // Run the session, evaluating our "c" operation from the graph
    pair <string,tensorflow::Tensor> p(input, *input_tensor);
    vector <std::pair<string,tensorflow::Tensor>> v{p};

    /* vector <std::pair<string,tensorflow::Tensor>> v{test_pair}; */

    status = session->Run(v, {"k2tfout_0"}, {}, &outputs);
    /* if (!status.ok()) { */
    /*   std::cout << status.ToString() << "\n"; */
    /* } */

    // Grab the first output (we only evaluated one graph node: "c")
    // and convert the node to a scalar representation.
    auto output_c = outputs[0].tensor<float,1>();
    float x;
    int col_counter = 0;
    
    for (int i=0; i < NUM_OUTPUTS; i++) {
      Mat measurement;

      x = output_c(i);
      x = x * (float)IMG_HEIGHT;
      measurement = (Mat_<float>(1, 1) << x);

      x = (kf[i]->predict()).at<float>(0,0);
      scan[i] = x;
      kf[i]->correct(measurement);

      if (1==1) {
        circle(new_image, Point(col_counter, (int)x), 2, Scalar(0,0,255), -1);
      }

      col_counter += (IMG_WIDTH / NUM_OUTPUTS);
    }

    //publish message
    sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
    image_pub_.publish(pub_msg);

    //publish_point_cloud();
  }

  void publish_point_cloud() {
    // Fill in new PointCloud2 message (2D image-like layout)
    sensor_msgs::PointCloud2 points_msg = sensor_msgs::PointCloud2();

    /* points_msg->header = disp_msg->header; */
    points_msg.header.stamp = ros::Time::now();
    points_msg.header.seq = scan_counter;
    points_msg.header.frame_id = "see3_cam";
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
    float cur_x = 5; //the starting x coordinate
    float z = .02; //the ground is a plane so the Z-coordinate is 0
    for (int u = 0; u < 48 ; u++) { //go through the 48 points
      float img_x = cur_x;
      float img_y = scan[u];

      double c_x,c_y;
      transform_point(img_x, img_y, &c_x, &c_y);
      float temp_x, temp_y;
      temp_x = -c_x;
      temp_y = c_y;

      if (temp_y < 0)
          temp_y = 0;

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

  void init_camera_transform() {
    //compute the inverse of the A matrix
    //self.r_t_inv = np.linalg.inv(self.A)
    cv::Mat temp(3,3,cv::DataType<double>::type);

    double v[3][3] = {
        { 0.000743783243792829 , 2.6596187378802284e-05 , -0.187180369550433 },
        { 8.775546286614852e-05 , -0.0029585196261919418 , 0.6466780875289659 },
        { -4.119315005340649e-05 , 0.0005354069190685872 , 0.08171436574498335 },
    };

    temp = cv::Mat(3,3,cv::DataType<double>::type, v);
    r_t_inv = temp.clone();
  }

  void transform_point(double x, double y, double* x_object, double* y_object) {
    /* imagepoint = np.array([x,y,1],dtype=np.float32) */
    double ip[3] = {x, y, 1.0000};
    cv::Mat imagepoint(3,1,cv::DataType<double>::type, ip);   

    /* ans = np.matmul(self.r_t_inv, imagepoint) */
    cv::Mat ans(3,1,cv::DataType<double>::type);   
    ans = r_t_inv * imagepoint;
    //cout << "M = "<< endl << " "  << ans << endl << endl;

    /* w = ans.item(2) */
    double w = ans.at<double>(2,0);

    /* ans = ans / w */
    ans = ans / w;

    /* return ans.tolist()[0:2] #answer is in meters */
    *x_object = ans.at<double>(0,0);
    *y_object = ans.at<double>(1,0);
  }

  ObstacleDetection() : it_(nh_) {
    if (1==1) {
      new_image = Mat(IMG_HEIGHT, IMG_WIDTH, CV_32FC(3));

      input_tensor = new tensorflow::Tensor(tensorflow::DT_FLOAT, 
         tensorflow::TensorShape({1, IMG_HEIGHT, IMG_WIDTH, 3})); 

      test_pair = make_pair(input, *input_tensor);
      test_v.push_back(test_pair);

      //float_t *p1 = input_tensor->flat<float_t>().data(); 

      image_transport::TransportHints hints("compressed");
      image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &ObstacleDetection::run_network, this, hints);
      /* image_sub_ = it_.subscribe("/zed_node/rgb/image_rect_color", 1, &ObstacleDetection::run_network, this); */
      image_pub_ = it_.advertise("/image_converter/output_video", 1);
      point_pub = nh_.advertise<sensor_msgs::PointCloud2>("/test_point_cloud", 1);
    }

    for (int i=0; i<NUM_OUTPUTS; i++) {
      kf[i] = new KalmanFilter(1,1); 

      Mat_<float> measurement(1,1); measurement.setTo(Scalar(0));

      kf[i]->statePre.at<float>(0) = 0;
      setIdentity(kf[i]->transitionMatrix);
      setIdentity(kf[i]->measurementMatrix);
      setIdentity(kf[i]->processNoiseCov, Scalar::all(1e-2));
      //setIdentity(kf[i]->measurementNoiseCov, Scalar::all(.1));
      setIdentity(kf[i]->measurementNoiseCov, Scalar::all(.0013));
      setIdentity(kf[i]->errorCovPost, Scalar::all(.1));
    }

    init_camera_transform();
    double x,y;
    transform_point(213/1.54,209/1.54, &x, &y);
    cout << "x:";
    cout << x/.0254;
    cout << "y:";
    cout << y/.0254;
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
    status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), ros::package::getPath("image_cnn") + "/models/output_graph_360.pb", &graph_def);
    if (!status.ok()) {
      std::cout << status.ToString() << "\n";
      return 1;
    }

    status = session->Create(graph_def);
    return 0;
  }
};

int main(int argc, char** argv) {
  ros::init(argc, argv, "image_cnn");
  ObstacleDetection obstacle_detect;
  obstacle_detect.init_tensorflow();
  ros::spin();
}
