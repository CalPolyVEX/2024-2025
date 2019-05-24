#!/bin/bash
if [ "$#" -ne 1 ]; then
   rosbag record --duration=2m /zed/data_throttled_image_depth /zed/data_throttled_image /zed/data_throttled_camera_info /camera/odom/sample /tf /tf_static 
   #rosbag record --duration=2m /zed/data_throttled_image_depth /zed/data_throttled_image /zed/data_throttled_camera_info /zed/depth/camera_info /zed/odom /tf /tf_static /zed/throttled_point_cloud
else
   echo "one argument"
   rosbag record --duration=2m /zed/data_throttled_image_depth/compressedDepth /zed/data_throttled_image/compressed /zed/data_throttled_camera_info /zed/depth/camera_info /camera/odom/sample /tf /tf_static
fi
