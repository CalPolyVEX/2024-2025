#!/bin/bash

export ROS_VERSION=kinetic #(Ubuntu 16.04: kinetic, Ubuntu 14.04: indigo)
export CATKIN_WS=~/orbslam_ws
mkdir -p $CATKIN_WS/src
cd $CATKIN_WS
catkin init
catkin config --merge-devel # Necessary for catkin_tools >= 0.4.
catkin config --extend /opt/ros/$ROS_VERSION
catkin config --cmake-args -DCMAKE_BUILD_TYPE=Release
cd src


git clone https://github.com/catkin/catkin_simple.git
git clone https://github.com/ethz-asl/orb_slam_2_catkin.git
git clone https://github.com/ethz-asl/image_undistort.git
git clone https://github.com/uzh-rpg/pangolin_catkin.git
git clone https://github.com/ethz-asl/orb_slam_2_ros.git
