#include "obstacle_cnn.h"

using namespace cv;
using namespace std;

void ObstacleDetection::run_network(Mat* m) {
  cv_bridge::CvImageConstPtr cv_ptr;
  tensorflow::Status status;

  float* imgTensorFlat = input_tensor->flat<float>().data(); 
  //imgTensorFlat = ((test_v.data())->second).flat<float>().data(); 
  unsigned char* p1;

  int test = 1;
  if (test == 1) {
    //resize image to IMG_HEIGHT x IMG_WIDTH
    cv::resize(*m, new_image, cv::Size(IMG_WIDTH,IMG_HEIGHT), CV_INTER_LINEAR);

    //fill in tensor with image data
    for(int i = 0; i < IMG_HEIGHT; i++) {
      p1 = (unsigned char*) m->ptr<unsigned char>(i);
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
    //cv::resize(*m, new_image, cv::Size(IMG_WIDTH,IMG_HEIGHT), CV_INTER_LINEAR);

    for (int y = 0; y < IMG_HEIGHT; y++) {
      for (int x = 0; x < IMG_WIDTH; x++) {
        Vec3b pixel = (*m).at<Vec3b>(y, x);

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

  status = session->Run(v, {"k2tfout_0"}, {}, &outputs);
  //status = session->Run(test_v, {"k2tfout_0"}, {}, &outputs);
  if (!status.ok()) {
    std::cout << status.ToString() << "\n";
  }

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

    col_counter += 5;
  }

  //publish message
  sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", new_image).toImageMsg();
  image_pub_.publish(pub_msg);
}

ObstacleDetection::ObstacleDetection() : it_(nh_) {
  new_image = Mat(IMG_HEIGHT, IMG_WIDTH, CV_32FC(3));

  input_tensor = new tensorflow::Tensor(tensorflow::DT_FLOAT, 
      tensorflow::TensorShape({1, IMG_HEIGHT, IMG_WIDTH, 3})); 

  test_pair = make_pair(input, *input_tensor);
  test_v.push_back(test_pair);

  //float_t *p1 = input_tensor->flat<float_t>().data(); 

  image_transport::TransportHints hints("compressed");
  //image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &ImageConverter::run_network, this);
  //image_sub_ = it_.subscribe("/see3cam_cu20/image_raw", 1, &ObstacleDetection::run_network, this, hints);
  /* image_sub_ = it_.subscribe("/zed/data_throttled_image", 1, &ImageConverter::run_network, this); */
  image_pub_ = it_.advertise("/image_converter/output_video", 1);

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
}

int ObstacleDetection::init_tensorflow() {
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
