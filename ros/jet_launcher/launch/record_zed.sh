#!/bin/sh
rosbag record --duration=2m /zed/rgb/image_rect_color/compressed /zed/depth/depth_registered/compressedDepth /zed/rgb/camera_info /zed/depth/camera_info /zed/odom /tf /tf_static
