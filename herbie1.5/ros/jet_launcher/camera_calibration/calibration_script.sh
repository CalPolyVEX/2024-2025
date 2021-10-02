#!/bin/bash

rosrun camera_calibration cameracalibrator.py --size 7x5 --square 0.030 image:=/camera/image_raw camera:=/camera
