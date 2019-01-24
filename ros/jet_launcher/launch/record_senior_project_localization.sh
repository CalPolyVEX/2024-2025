#!/bin/bash
if [ "$#" -ne 1 ]; then
   rosbag record --duration=2m /tf /tf_static /see3cam_cu20/image_raw/compressed
else
   echo "one argument"
   rosbag record --duration=2m /zed/data_throttled_image_depth/compressedDepth /zed/data_throttled_image/compressed /zed/data_throttled_camera_info /zed/depth/camera_info /zed/odom /tf /tf_static
fi
