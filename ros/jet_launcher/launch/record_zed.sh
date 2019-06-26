#!/bin/bash
if [ "$#" -ne 1 ]; then
   rosbag record --duration=2m /zed/data_throttled_image_depth /zed/data_throttled_image /zed/data_throttled_camera_info /tf /tf_static /ekf_node/odom
else
   echo "one argument"
   rosbag record --duration=2m /zed/data_throttled_image_depth/compressedDepth /zed/data_throttled_image/compressed /zed/data_throttled_camera_info /zed/depth/camera_info /camera/odom/sample /tf /tf_static
fi
