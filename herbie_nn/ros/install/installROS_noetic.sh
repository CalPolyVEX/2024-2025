#!/bin/bash

# Setup Locale
# sudo update-locale LANG=C LANGUAGE=C LC_ALL=C LC_MESSAGES=POSIX
# Setup sources.lst
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
# Setup keys
curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -

# Installation
sudo apt-get update
sudo apt-get install git ros-noetic-ros-base -y

# Initialize rosdep
sudo apt-get install python3-rosdep -y

# Initialize rosdep
sudo rosdep init
# To find available packages, use:
rosdep update
# Environment Setup
echo "source /opt/ros/noetic/setup.bash" >> ~/.bashrc
source ~/.bashrc
# Install rosinstall
sudo apt-get install python3-rosinstall -y

#setup workspace
mkdir -p ~/catkin_ws/src
cd ~/catkin_ws/src
git init
git config core.sparseCheckout true
echo 'herbie_nn/' > .git/info/sparse-checkout
git remote add -f origin https://github.com/jsseng/ue4.git
git pull origin master
git clone https://github.com/ros-perception/image_transport_plugins.git

#vision_opencv
git clone https://github.com/ros-perception/vision_opencv.git
#For OpenCV4 compatibility
#git clone https://github.com/BrutusTT/vision_opencv.git
#git checkout 8e01b44c5c1c0003dc91273076f8ca7feb9a8025

git clone https://github.com/ros-drivers/video_stream_opencv.git
git clone https://github.com/ros-drivers/libuvc_ros.git

#install dependencies
sudo apt install python-pygame gfortran libogg-dev libtheora-dev
sudo apt install ros-noetic-rosserial-arduino ros-noetic-tf-conversions ros-noetic-eigen-conversions
sudo apt install ros-noetic-tf2-sensor-msgs ros-noetic-tf2-geometry-msgs ros-noetic-laser-geometry
sudo apt install ros-noetic-roslint ros-noetic-image-transport ros-noetic-diagnostic-updater
sudo apt install python3-pip ros-noetic-pcl-conversions ros-noetic-pcl-ros ros-noetic-robot-localization
sudo apt install python3-pip ros-noetic-move-base-msgs ros-noetic-costmap-2d ros-noetic-octomap-msgs ros-noetic-find-object-2d ros-noetic-camera-info-manager
sudo apt install ros-noetic-rviz ros-noetic-serial libuvc-dev ros-noetic-rqt-common-plugins
sudo apt install ros-noetic-move-base ros-move-base-msgs ros-noetic-global-planner

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

if [ $1 -gt 100 ]  #if running on a Jetson board
then
   #disable graphical boot
   systemctl set-default multi-user.target

   #disable services
   systemctl disable ModemManager.service
   systemctl disable containerd.service
   sudo systemctl disable docker.service
   sudo systemctl disable docker.socket
fi
