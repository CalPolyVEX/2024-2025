#!/usr/bin/python

import os,sys

if (len(sys.argv)) > 1:
   model_name = sys.argv[1]
   print 'Using model: ' + model_name
else:
   model_name = 'k_test3_mobilenet.py_model.h5'
   print 'Using default model: ' + model_name

os.system('scp jseng@unix3.csc.calpoly.edu:/home/jseng/ue4/ground_detection/gpu_code/saved_models/' + model_name + ' .')

os.system('python k2tf_convert.py --model ./' + model_name + ' --numout 48')
os.system('cp output_graph.pb /home/nvidia/catkin_ws/src/ros/image_test/nodes')
