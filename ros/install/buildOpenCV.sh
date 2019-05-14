#!/bin/bash

cd /mnt/temp
wget -O opencv.zip https://github.com/opencv/opencv/archive/4.1.0.zip
wget -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/4.1.0.zip
unzip opencv.zip
unzip opencv_contrib.zip
mv opencv-4.1.0 opencv
mv opencv_contrib-4.1.0 opencv_contrib

cd opencv
mkdir build
cd build
cmake -D WITH_CUDA=ON -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D INSTALL_PYTHON_EXAMPLES=ON -D INSTALL_C_EXAMPLES=OFF -D OPENCV_ENABLE_NONFREE=ON -D OPENCV_EXTRA_MODULES_PATH=/mnt/temp/opencv_contrib/modules -D PYTHON2_EXECUTABLE=/usr/bin/python -D PYTHON3_EXECUTABLE=/usr/bin/python3 -D BUILD_EXAMPLES=OFF ..

