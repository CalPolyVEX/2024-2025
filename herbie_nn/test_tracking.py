#!/usr/bin/env python3
from __future__ import print_function

import roslib, rosbag
import sys, rospy, cv2, time, queue
import numpy as np
from sensor_msgs.msg import CompressedImage
import torch
from torchvision import transforms
import math

class image_inference:
   def mouse_click(self,event,x,y,flags,param):
      if event == cv2.EVENT_LBUTTONDOWN:
         self.mouseX, self.mouseY = x,y
         self.target_set = 1

         self.tracker = cv2.TrackerKCF_create()

         box_width = 100
         bbox = (self.mouseX- int(box_width/2), self.mouseY - int(box_width/2), box_width, box_width)

         self.tracker.init(self.image_np, bbox)
         self.tracking = 1
         print ((x,y))

   def __init__(self, model_name = "mobilenetv2_100-.010-80out.pt"):
      #create the window

      #self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw/compressed",CompressedImage,self.callback, queue_size=1, buff_size=10000000)
      self.tracker = cv2.TrackerKCF_create()

      self.point_list = []
      self.image_np_360 = None
      self.image_np = None
      self.counter = 0
      self.tracking = 0

      self.mouseX = 0
      self.mouseY = 0
      self.target_set = 0
      self.key = 0

   def read_bag(self):
      bag = rosbag.Bag('/home/jseng/ref_bags/output10.bag')
      counter = 0
      cv2.setMouseCallback('image',self.mouse_click)

      for topic, msg, t in bag.read_messages(topics=['/see3cam_cu20/image_raw/compressed']):
         #print(msg)

         np_arr = np.frombuffer(msg.data, np.uint8)
         self.image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
         self.image_np = cv2.resize(self.image_np, (640, 360))  #resize

         if self.tracking == 0:
            self.tracker = cv2.TrackerKCF_create()
            # self.tracker = cv2.TrackerCSRT_create()

            #wait for a mouse click
            local_mousex = 0
            local_mousey = 0
            exit_loop = 0

            local_mousex = self.mouseX
            local_mousey = self.mouseY

            box_width = 100
            bbox = (local_mousex - int(box_width/2), local_mousey - int(box_width/2), box_width, box_width)

            self.tracker.init(self.image_np, bbox)
            self.tracking = 1
         else:
            if self.tracking==1:
               (success, box) = self.tracker.update(self.image_np)
               # check to see if the tracking was a success
               if success:
                  (x, y, w, h) = [int(v) for v in box]
                  cv2.rectangle(self.image_np, (x, y), (x + w, y + h), (0, 255, 0), 2)
                  self.tracking = 1
               else:
                  print ('tracking lost')
                  self.tracking = 0
                  self.target_set = 0
                  while self.target_set == 0:
                     cv2.waitKey(1)

         cv2.imshow('image',self.image_np)

         #hit 'q' to quit, 'p' to pause
         playing = 1
         paused = 0

         while playing == 1 or paused == 1:
            key = cv2.waitKey(50) 
            if key == ord('q'):
               bag.close()
               cv2.destroyAllWindows()
               return
            elif key == ord('p'):
               if paused == 1:
                  paused = 0
               elif paused==0:
                  paused = 1
            elif paused==0:
               playing=0

         counter += 1

      #close the bag file and close windows
      bag.close()
      cv2.destroyAllWindows()


def main(args):
   global mouseX, mouseY

   rospy.init_node('inference_node', anonymous=True)
   cv2.namedWindow('image',cv2.WINDOW_NORMAL)

   if len(args) == 2:
       ic = image_inference(model_name = args[1])
   else:
       ic = image_inference()

   ic.read_bag()
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
