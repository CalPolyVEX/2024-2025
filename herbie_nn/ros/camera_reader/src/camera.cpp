#include "camera.h"

ros::NodeHandle* nh = NULL; 

int CameraReader::init(int argc, char **argv, ros::NodeHandle* nh) {
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

   yuyv_frame = Mat(height, width, CV_8UC2);        //full HD resolution
   bgr_frame = Mat(height, width, CV_8UC3);         //full HD resolution
   bgr_frame_360 = Mat(height/3, width/3, CV_8UC3); //640x360 resolution

   return 0;
}

void CameraReader::frame_loop() {
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

#ifndef ARM_CPU
         cvtColor(yuyv_frame, bgr_frame, COLOR_YUV2BGR_UYVY);
         resize(bgr_frame, bgr_frame_360, bgr_frame_360.size(), 0, 0);
         /* resize(preview, preview1, preview1.size(), 0, 0); */
#else
         //if this CPU supports NEON
         asm_foo((unsigned char*) yuyv_frame.data, (unsigned char*) bgr_frame_360.data);
#endif

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

int CameraReader::close_camera() {
   /*
    * Helper function to free allocated resources and close the camera device.
    */
   if (helper_deinit_cam() < 0)
   {
      return EXIT_FAILURE;
   }

   return 0;
}

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

  ros::spin();
  ros::waitForShutdown();
  return EXIT_SUCCESS;
}
