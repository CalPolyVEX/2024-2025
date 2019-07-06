#!/bin/bash

# Setup Locale
# sudo update-locale LANG=C LANGUAGE=C LC_ALL=C LC_MESSAGES=POSIX
# Setup sources.lst
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
# Setup keys
sudo apt-key adv --keyserver hkp://ha.pool.sks-keyservers.net:80 --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654

# Installation
sudo apt-get update
sudo apt-get install git ros-melodic-ros-base -y

# Initialize rosdep
sudo apt-get install python-rosdep -y

# Initialize rosdep
sudo rosdep init
# To find available packages, use:
rosdep update
# Environment Setup
echo "source /opt/ros/melodic/setup.bash" >> ~/.bashrc
source ~/.bashrc
# Install rosinstall
sudo apt-get install python-rosinstall -y

#setup workspace
mkdir -p ~/catkin_ws/src
cd ~/catkin_ws/src
git init
git config core.sparseCheckout true
echo 'ros/' > .git/info/sparse-checkout
git remote add -f origin https://github.com/jsseng/ue4.git
git pull origin master
git clone https://github.com/ros-perception/image_transport_plugins.git
git clone https://github.com/ros-perception/vision_opencv.git
git clone https://github.com/ros-drivers/video_stream_opencv.git
git clone https://github.com/IntelRealSense/realsense-ros.git
git clone https://github.com/introlab/rtabmap_ros.git
git clone https://github.com/stereolabs/zed-ros-wrapper.git

#install dependencies
sudo apt install python-pygame gfortran libogg-dev libtheora-dev
sudo apt install ros-melodic-rosserial-arduino ros-melodic-tf-conversions ros-melodic-eigen-conversions
sudo apt install ros-melodic-tf2-sensor-msgs ros-melodic-tf2-geometry-msgs ros-melodic-laser-geometry
sudo apt install ros-melodic-roslint ros-melodic-image-transport ros-melodic-diagnostic-updater
sudo apt install python-pip ros-melodic-pcl-conversions ros-melodic-pcl-ros ros-melodic-dynamic-robot-state-publisher ros-melodic-robot-localization
sudo apt install python3-pip ros-melodic-move-base-msgs ros-melodic-costmap-2d ros-melodic-octomap-msgs ros-melodic-find-object-2d ros-melodic-camera-info-manager

#install Tensorflow for python3 (Jetpack 4.2)
#pip3 install --extra-index-url https://developer.download.nvidia.com/compute/redist/jp/v42 tensorflow-gpu==1.13.1+nv19.4 --user
#pip install numpy
#pip3 install numpy

#install vim pathogen
sudo apt install curl
mkdir -p ~/.vim/autoload ~/.vim/bundle
curl -LSso ~/.vim/autoload/pathogen.vim https://tpo.pe/pathogen.vim

#config git
git config --global user.email "jseng@calpoly.edu"
git config --global user.name "John Seng"

#stuff for building OpenCV
#sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
#sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libdc1394-22-dev


