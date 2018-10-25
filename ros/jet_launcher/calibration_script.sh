#!/bin/bash

rosrun camera_calibration cameracalibrator.py --size 7x5 --square 0.30 image:=/see3cam_cu20/image_raw camera:=/see3cam_cu20
