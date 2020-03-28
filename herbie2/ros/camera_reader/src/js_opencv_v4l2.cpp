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
 
using namespace std;
using namespace cv;

ros::NodeHandle* nh = NULL;

/*
 * Other formats: To use pixel formats other than UYVY, see related comments (comments with
 * prefix 'Other formats') in corresponding places.
 */
class CameraReader {
  unsigned int width, height;
  const char* default_videodev = "/dev/video0";
  const char *videodev;
  unsigned int start, end, fps = 0;
  unsigned char* ptr_cam_frame;
  int bytes_used;
  ros::NodeHandle nh_;
  ros::NodeHandle* n;
  image_transport::ImageTransport it_;
  image_transport::Publisher image_pub_;

  /*
   * Re-using the frame matrix(ces) instead of creating new ones (i.e., declaring 'Mat frame'
   * (and cuda::GpuMat gpu_frame) outside the 'while (1)' loop instead of declaring it
   * within the loop) improves the performance for higher resolutions.
   */
  Mat yuyv_frame, bgr_frame, bgr_frame_360;

  public:
  CameraReader() : it_(nh_) {
    image_pub_ = it_.advertise("/see3cam_cu20/image_raw", 1);
  }

  int init(int argc, char **argv, ros::NodeHandle* nh) {
    n = nh;
    if (argc == 4) {
      videodev = argv[1];

      string width_str = argv[2];
      string height_str = argv[3];
      try {
        size_t pos;
        width = stoi(width_str, &pos);
        if (pos < width_str.size()) {
          cerr << "Trailing characters after width: " << width_str << '\n';
        }

        height = stoi(height_str, &pos);
        if (pos < height_str.size()) {
          cerr << "Trailing characters after height: " << height_str << '\n';
        }
      } catch (invalid_argument const &ex) {
        cerr << "Invalid width or height\n";
        return EXIT_FAILURE;
      } catch (out_of_range const &ex) {
        cerr << "Width or Height out of range\n";
        return EXIT_FAILURE;
      }
    } else {
      cout << "Note: This program accepts (only) three arguments.\n";
      cout << "First arg: device file path, Second arg: width, Third arg: height\n";
      cout << "No arguments given. Assuming default values.\n";
      cout << "Device file path: " << default_videodev << "; Width: 640; Height: 480\n";
      videodev = default_videodev;
      width = 1920;
      height = 1080;
    }

    /*
     * Helper function to initialize camera to a specific resolution and format
     *
     * 1. Using User pointer method of streaming I/O contributes to increased performance as
    *    it avoids a spurious copy from kernel space to user space which happens in case of
    *    the Memory mapping streaming I/O method.
    *
    * 2. Other formats: To use formats other that UYVY, the 4th parameter for this function
    *    must be modified accordingly to pass a valid value for the 'pixelformat' member of
    *    'struct v4l2_pix_format'[1].
    *
    * [1]: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/pixfmt-v4l2.html#c.v4l2_pix_format
    */
   /* if (helper_init_cam(videodev, width, height, V4L2_PIX_FMT_UYVY, IO_METHOD_USERPTR) < 0) { */
   if (helper_init_cam(videodev, width, height, V4L2_PIX_FMT_UYVY, IO_METHOD_MMAP) < 0) {
      return EXIT_FAILURE;
   }

   /*
    * 1. As we re-use the matrix across loops for increased performance in case of higher resolutions
    *    we construct it with the common parameters: rows (height), columns (width), type of data in
    *    matrix.
    *
    *    Re-using the matrix is possible as the resolution of the frame doesn't change dynamically
    *    in the middle of obtaining frames from the camera. If resolutions change in the middle we
    *    would have to re-construct the matrix accordingly.
    *
    * 2. Other formats: To use formats other than UYVY the 3rd parameter must be modified accordingly
    *    to pass a valid OpenCV array type for the new pixelformat[2].
    *
    * [2]: https://docs.opencv.org/3.4.2/d3/d63/classcv_1_1Mat.html#a2ec3402f7d165ca34c7fd6e8498a62ca
    */
   yuyv_frame = Mat(height, width, CV_8UC2);
   bgr_frame = Mat(height, width, CV_8UC3);
   bgr_frame_360 = Mat(height/3, width/3, CV_8UC3);

   return 0;
  }

  void frame_loop() {
    while(ros::ok()) {
        usleep(88000);
        bool read_cam;
        n->getParam("/read_see3cam", read_cam);
        if (read_cam == true) { 
          //usleep(40000);
          /*
           * Helper function to access camera data
           */
          if (helper_get_cam_frame(&ptr_cam_frame, &bytes_used) < 0) {
            break;
          }

          /*
           * It's easy to re-use the matrix for our case (V4L2 user pointer) by changing the
           * member 'data' to point to the data obtained from the V4L2 helper.
           */
          yuyv_frame.data = ptr_cam_frame;
          if(yuyv_frame.empty()) {
            cout << "Img load failed" << endl;
            break;
          }

          cvtColor(yuyv_frame, bgr_frame, COLOR_YUV2BGR_UYVY);
          resize(bgr_frame, bgr_frame_360, bgr_frame_360.size(), 0, 0);
          /* resize(preview, preview1, preview1.size(), 0, 0); */

          sensor_msgs::ImagePtr pub_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", bgr_frame_360).toImageMsg();
          image_pub_.publish(pub_msg);

          /*
           * Helper function to release camera data. This must be called for every
           * call to helper_get_cam_frame()
           */
          if (helper_release_cam_frame() < 0) {
            break;
          }
        }
    }
  }

  int close_camera() {
    /*
     * Helper function to free allocated resources and close the camera device.
     */
    if (helper_deinit_cam() < 0)
    {
      return EXIT_FAILURE;
    }

    return 0;
  }
};

void mySigintHandler(int sig)
{
  helper_release_cam_frame();

  usleep(10000);
  helper_deinit_cam();  //close the camera
  
  // All the default sigint handler does is call shutdown()
  ros::shutdown();
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "camera_reader_node", ros::init_options::NoSigintHandler);
  ros::NodeHandle n;
  nh = &n;

  signal(SIGINT, mySigintHandler);

  CameraReader c;
  nh->setParam("/read_see3cam", false);
  c.init(argc,argv,nh);
  c.frame_loop();

  //c.close_camera();

  ros::spin();
  ros::waitForShutdown();
  return EXIT_SUCCESS;
}
