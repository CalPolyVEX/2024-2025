#include "camera.h"
#include <getopt.h>
#include <string>

ros::NodeHandle* nh = NULL; 

int CameraReader::init(char* videodev, int width, int height, ros::NodeHandle* nh) {
   n = nh;

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
   int c, accel=0;
   int verbose_flag, width=1920, height=1080;
   int build_flag=0, load_flag=0;
   char videodev[50], onnx_file[200], engine_file[200];
   videodev[0] = 0;

   while (1)
   {
      static struct option long_options[] =
      {
         /* These options set a flag. */
         //{"load",   no_argument,      &verbose_flag, 0},
         /* These options don’t set a flag.
            We distinguish them by their indices. */
         {"accelerator", required_argument, 0, 'a'},
         {"build",   required_argument, 0, 'b'},
         {"device",  required_argument, 0, 'd'},
         {"width",   required_argument, 0, 'w'},
         {"height",  required_argument, 0, 'h'},
         {"load",    required_argument, 0, 'l'},
         {0, 0, 0, 0}
      };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "a:b:d:w:h:l:",
            long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
         break;

      switch (c)
      {
      case 0:
         /* If this option set a flag, do nothing else now. */
         if (long_options[option_index].flag != 0)
            break;
         printf ("option %s", long_options[option_index].name);
         if (optarg)
            printf (" with arg %s", optarg);
         printf ("\n");
         break;

      case 'a':
         printf ("Using accelerator: '%s'\n", optarg);
         accel = stoi(optarg);
         break;
         
      case 'b':
         printf ("Building engine with ONNX file: '%s'\n", optarg);
         strncpy(onnx_file,optarg,200);
         build_flag = 1;
         break;

      case 'l':
         printf ("Loading engine file: '%s'\n", optarg);
         strncpy(engine_file,optarg,200);
         load_flag = 1;
         break;
         
      case 'd':
         strncpy(videodev,optarg,50);
         break;

      case 'w':
         printf ("option -w with value `%s'\n", optarg);
         width = stoi(optarg);
         break;

      case 'h':
         printf ("option -h with value `%s'\n", optarg);
         height = stoi(optarg);
         break;

      case '?':
         /* getopt_long already printed an error message. */
         break;

      default:
         abort ();
      }
   }

  //if there is a video device, then run in camera reader mode
  if (strlen(videodev) > 0) {
     std::cout << "Using device: " << videodev << std::endl;
     std::cout << "Using width: " << width << std::endl;
     std::cout << "Using height: " << height << std::endl;

     ros::init(argc, argv, "camera_reader_node", ros::init_options::NoSigintHandler);
     ros::NodeHandle n;
     nh = &n;

     signal(SIGINT, mySigintHandler);

     CameraReader cr;
     nh->setParam("/read_see3cam", false);
     cr.init(videodev,width,height,nh);
     cr.frame_loop();

     ros::spin();
     ros::waitForShutdown();
     return EXIT_SUCCESS;
  } else if (build_flag == 1) {
     std::cout << "Building engine..." << std::endl;
     CameraReader::buildEngine(onnx_file, accel);
  } else if (load_flag == 1) {
     std::cout << "Loading engine..." << std::endl;
     CameraReader::loadEngine(engine_file);
  }

  return 0;
}
