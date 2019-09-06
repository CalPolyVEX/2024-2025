//6/1/19 This file receives gyro messages from the T265 and change the 
//frame_id to a frame that is fixed to the base_link frame

#include <ros/ros.h>
#include <ros/package.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
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
  cv::Mat r_t_inv;
  
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

  void init_camera_transform() {
    std::vector<cv::Point3d> objectPoints;
    double objectPoints_array[17][3] = {
      {18,0,45}, //these points are measured on the ground (in inches) 
      {27,0,27}, 
      {9,0,63}, 
      {54,0,54}, 
      {0,0,36},
      {-18,0,36},
      {-18,0,45},
      {-18,0,63},
      {0,0,90},
      {-18,0,90},
      //data after this line gathered on 11/3/18
      {0,0,120},
      {-36,0,120},
      {48,0,120},
      {0,0,180},
      {48,0,180},
      {48,0,41},
      {0,0,240}
    };

    std::vector<cv::Point2d> imagePoints;
    double imagePoints_array[17][3] = {
      {527,339}, //740x415 calibrate script 
      {673,409}, 
      {424,285}, 
      {708,283}, 
      {357,394},
      {164,380},
      {192,339},
      {228,284},
      {360,232},
      {264,235},
      //data after this line gathered on 11/3/18
      {356,207},
      {213,209},
      {538,203},
      {360,175},
      {487,173},
      {733,315},
      {358,161}
    };

    //intrinsic matrix
    double cameraMatrix[3][3] = {
      {1258.513767, 0.000000, 949.143263},
      {0.000000, 1260.515476, 587.553871},
      {0.000000, 0.000000, 1.000000}
    };

    //distortion coefficients
    double distCoeffs[5][1] = {-0.350545, 0.098685, -0.004605, -0.001945, 0.000000};

    //create the matrices
    cv::Mat cameraMatrix_mat(3,3,cv::DataType<double>::type,cameraMatrix);
    cv::Mat distCoeffs_mat(5,1,cv::DataType<double>::type,distCoeffs);

    //create the point vectors
    for (int i=0;i<17;i++) {
      double temp_x = objectPoints_array[i][0] * .0254; //convert to meters 
      double temp_y = 0; 
      double temp_z = objectPoints_array[i][2] * .0254; 

      double temp_image_x = imagePoints_array[i][0] / 1.54; //scale to 480x270
      double temp_image_y = imagePoints_array[i][1] / 1.54; 
      
      objectPoints.push_back(cv::Point3d(temp_x, temp_y, temp_z));
      imagePoints.push_back(cv::Point2d(temp_image_x, temp_image_y));
    }

    //scale to 480x270, so divide by 4
    cameraMatrix_mat = cameraMatrix_mat / 4.000;

    //create rvec and tvec
    cv::Mat rvec(3,1,cv::DataType<double>::type);
    cv::Mat tvec(3,1,cv::DataType<double>::type);

    //self.retval, self.rvec, self.tvec = cv2.solvePnP(self.objectPoints, self.imagePoints, self.cameraMatrix, self.distCoeffs, flags=cv2.SOLVEPNP_ITERATIVE)
    cv::solvePnP(objectPoints, imagePoints, cameraMatrix_mat, distCoeffs_mat, rvec, tvec);
    
    //self.rmat, self.rmat_jacobian = cv2.Rodrigues(self.rvec)
    cv::Mat rmat(3,3,cv::DataType<double>::type);
    cv::Mat rmat_jacobian(3,9,cv::DataType<double>::type);
    cv::Rodrigues(rvec, rmat, rmat_jacobian);
    
    //concatenate r and t matrix
    //self.r_t_max = np.concatenate((self.rmat,self.tvec), axis=1)
    cv::Mat r_t_mat(3,4,cv::DataType<double>::type);
    cv::hconcat(rmat, tvec, r_t_mat);

    //#multiply intrinsic matrix with R|t matrix
    //self.A = np.matmul(self.cameraMatrix, self.r_t_max)
    cv::Mat A(3,4,cv::DataType<double>::type);
    A = cameraMatrix_mat * r_t_mat;

    //remove the second column because it is a plane
    //self.A = np.delete(self.A,1,1)
    cv::Mat A_plane = cv::Mat(3,3,cv::DataType<double>::type);
    /* cv::Mat A_plane = A.col(0); */
    /* cv::hconcat(A_plane, A.col(2), A_plane); */
    /* cv::hconcat(A_plane, A.col(3), A_plane); */
    //cout << "---" << A_plane.rows << "-----" << A_plane.cols << "-----";
    A.col(0).copyTo(A_plane.col(0));
    A.col(2).copyTo(A_plane.col(1));
    A.col(3).copyTo(A_plane.col(2));

    //compute the inverse of the A matrix
    //self.r_t_inv = np.linalg.inv(self.A)
    cv::Mat temp(3,3,cv::DataType<double>::type);
    temp = A_plane.inv();

    double v[3][3] = {
      { 0.0031784365485609833 , -8.632513240555703e-07 , -0.7390697592912845 },
      { -4.4643917161382675e-05 , -0.0005914395002240171 , 1.0808936509326386 },
      { 7.418011022223405e-05 , 0.005344582012109148 , -0.4032597845481824 }};

    temp = cv::Mat(3,3,cv::DataType<double>::type, v);
    r_t_inv = temp.clone();
  }

  void transform_point(double x, double y, double* x_object, double* y_object) {
    /* def compute(self,x,y): */
    /*    imagepoint = np.array([x,y,1],dtype=np.float32) */
    double ip[3] = {x, y, 1.0000};
    cv::Mat imagepoint(3,1,cv::DataType<double>::type, ip);   

    /*    ans = np.matmul(self.r_t_inv, imagepoint) */
    cv::Mat ans(3,1,cv::DataType<double>::type);   
    ans = r_t_inv * imagepoint;
    cout << "M = "<< endl << " "  << ans << endl << endl;

    /*    w = ans.item(2) */
    double w = ans.at<double>(2,0);

    /*    ans = ans / w */
    ans = ans / w;

    /*    #ans = ans / .0254 #convert back to inches */

    /*    return ans.tolist()[0:2] #answer is in meters */
    *x_object = ans.at<double>(0,0);
    *y_object = ans.at<double>(1,0);
  }

  ImageConverter() : it_(nh_) {
    if (1==0) {
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

    r_t_inv = cv::Mat(3,3, cv::DataType<double>::type);
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
  //gyro_pub.init_tensorflow();
  //ros::spin();
}
