#!/bin/bash
./source_sync.sh -k tegra-l4t-r28.2.1

cd sources/kernel/kernel-4.4

#make clean
cp /proc/config.gz .
gunzip config.gz
cp config .config
../../../configureKernel.sh

cp /mnt/temp/librealsense/scripts/realsense-camera-formats_ubuntu-xenial.patch .
cp /mnt/temp/librealsense/scripts/v1-media-uvcvideo-mark-buffer-error-where-overflow.patch .
cp /mnt/temp/librealsense/scripts/realsense-powerlinefrequency-control-fix.patch .
cp /mnt/temp/librealsense/scripts/realsense-hid-ubuntu-xenial-master.patch .
cp /mnt/temp/librealsense/scripts/realsense-metadata-ubuntu-xenial-master.patch .

patch -p1 < realsense-camera-formats_ubuntu-xenial.patch 
patch -p1 < realsense-metadata-ubuntu-xenial-master.patch 
patch -p1 < realsense-hid-ubuntu-xenial-master.patch 
patch -p1 < realsense-powerlinefrequency-control-fix.patch 
patch -p1 < v1-media-uvcvideo-mark-buffer-error-where-overflow.patch 

make prepare
make modules_prepare
make -j6 Image
make modules
