#!/bin/bash
#rosbag record /camera/fisheye1/image_raw/compressed /camera/odom/sample /see3cam_cu20/image_raw/compressed /tf /tf_static /camera/gyro/imu_info /camera/gyro/sample
rosbag record --duration=120s /camera/odom/sample /see3cam_cu20/image_raw/compressed /tf /tf_static /camera/gyro/imu_info /camera/gyro/sample
