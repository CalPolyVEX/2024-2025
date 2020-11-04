#!/bin/bash

OPENCV=4.4.0

cd /mnt/temp/test_opencv
wget -O opencv.zip https://github.com/opencv/opencv/archive/$OPENCV.zip
wget -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/$OPENCV.zip
unzip opencv.zip
unzip opencv_contrib.zip
mv opencv-$OPENCV opencv
mv opencv_contrib-$OPENCV opencv_contrib

cd opencv
mkdir build
cd build
#cmake -D WITH_CUDA=ON -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D INSTALL_PYTHON_EXAMPLES=ON -D INSTALL_C_EXAMPLES=OFF -D OPENCV_ENABLE_NONFREE=ON -D OPENCV_EXTRA_MODULES_PATH=/mnt/temp/opencv_contrib/modules -D PYTHON2_EXECUTABLE=/usr/bin/python -D PYTHON3_EXECUTABLE=/usr/bin/python3 -D BUILD_EXAMPLES=OFF ..
cmake -D WITH_CUDA=ON -D CUDA_ARCH_BIN="7.2" -D CUDA_ARCH_PTX="" \
        -D WITH_CUBLAS=ON -D ENABLE_FAST_MATH=ON -D CUDA_FAST_MATH=ON \
        -D ENABLE_NEON=ON -D WITH_LIBV4L=ON -D CMAKE_BUILD_TYPE=RELEASE \
        -D CMAKE_INSTALL_PREFIX=/usr/local -D INSTALL_PYTHON_EXAMPLES=ON -D INSTALL_C_EXAMPLES=OFF \
        -D OPENCV_ENABLE_NONFREE=ON \
        -D OPENCV_EXTRA_MODULES_PATH=/mnt/temp/test_opencv/opencv_contrib/modules \
        -D PYTHON2_EXECUTABLE=/usr/bin/python -D PYTHON3_EXECUTABLE=/usr/bin/python3 \
        -D BUILD_EXAMPLES=OFF ..

