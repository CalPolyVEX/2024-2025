#!/bin/bash

# Setup Locale
# sudo update-locale LANG=C LANGUAGE=C LC_ALL=C LC_MESSAGES=POSIX
# Setup sources.lst
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
# Setup keys
sudo apt-key adv --keyserver hkp://ha.pool.sks-keyservers.net:80 --recv-key 421C365BD9FF1F717815A3895523BAEEB01FA116
# Installation
sudo apt-get update
sudo apt-get install ros-melodic-ros-base -y

# Initialize rosdep
sudo apt-get install python-rosdep -y
# ssl certificates can get messed up on TX1 for some reason
#sudo c_rehash /etc/ssl/certs
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

#install dependencies
sudo apt install python-pygame
sudo apt install ros-melodic-rosserial-arduino
sudo apt install ros-melodic-tf2-sensor-msgs
sudo apt install ros-melodic-roslint
sudo apt install python-pip
sudo apt install python3-pip

#install Tensorflow for python3 (Jetpack 4.2)
pip3 install --extra-index-url https://developer.download.nvidia.com/compute/redist/jp/v42 tensorflow-gpu==1.13.1+nv19.4 --user

#install vim pathogen
sudo apt install curl
mkdir -p ~/.vim/autoload ~/.vim/bundle
curl -LSso ~/.vim/autoload/pathogen.vim https://tpo.pe/pathogen.vim

#config git
git config --global user.email "jseng@calpoly.edu"
git config --global user.name "John Seng"

