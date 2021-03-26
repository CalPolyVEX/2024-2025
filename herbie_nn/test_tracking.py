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

         #reinitialize the tracker on a mouse click
         self.tracker = cv2.TrackerKCF_create()

         bbox = (self.mouseX- int(self.box_width/2), self.mouseY - int(self.box_width/2), self.box_width, self.box_width)

         self.tracker.init(self.image_np, bbox)
         self.tracking = 1
         print ((x,y))
         cv2.circle(self.image_np, (x,y), 4, (0,255,255), -1)
         cv2.imshow('image',self.image_np)

         #save image data here
         self.counter += 1
         print (self.counter)

   def __init__(self): 
      #create the window

      self.tracker = cv2.TrackerKCF_create()

      self.image_np = None
      self.counter = 0
      self.tracking = 0

      self.mouseX = 0
      self.mouseY = 0
      self.target_set = 0
      self.key = 0
      self.box_width = 100

   def read_bag(self, bag_name = '/home/jseng/ref_bags/2021-03-23-08-06-14.bag'):
      bag = rosbag.Bag(bag_name)
      cv2.setMouseCallback('image',self.mouse_click)

      for topic, msg, t in bag.read_messages(topics=['/see3cam_cu20/image_raw/compressed', '/camera/odom/sample']):
         if topic == '/see3cam_cu20/image_raw/compressed':
            np_arr = np.frombuffer(msg.data, np.uint8)
            self.image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
            self.image_np = cv2.resize(self.image_np, (640, 360))  #resize
            self.image_orig = self.image_np.copy() #the data to save
            self.image_orig_jpg = cv2.imencode('.jpg', self.image_orig)

            if self.tracking == 0:
               self.tracker = cv2.TrackerKCF_create()
               # self.tracker = cv2.TrackerCSRT_create()

               bbox = (self.mouseX - int(self.box_width/2), self.mouseY- int(self.box_width/2), self.box_width, self.box_width)

               #initialize tracker
               self.tracker.init(self.image_np, bbox)
               self.tracking = 1
               cv2.circle(self.image_np, (self.mouseX-int(self.box_width/2),self.mouseY-int(self.box_width/2)), 4, (0,255,255), -1)
               cv2.imshow('image',self.image_np)
            else:
               (success, box) = self.tracker.update(self.image_np)
               # check to see if the tracking was a success
               if success:
                  (x, y, w, h) = [int(v) for v in box]
                  cv2.rectangle(self.image_np, (x, y), (x + w, y + h), (0, 255, 0), 2)
                  self.tracking = 1
                  cv2.circle(self.image_np, (x+int(w/2),y+int(h/2)), 4, (0,255,255), -1)
                  cv2.imshow('image',self.image_np)

                  #save image data here
                  self.counter += 1
                  print (self.counter)
               else:
                  print ('tracking lost')
                  self.tracking = 0
                  self.target_set = 0
                  while self.target_set == 0:
                     cv2.waitKey(1)


            #hit 'q' to quit, 'z' to pause
            playing = 1
            paused = 0

            while playing == 1 or paused == 1:
               key = cv2.waitKey(50) 
               if key == ord('q'):
                  bag.close()
                  cv2.destroyAllWindows()
                  return
               elif key == ord('z'):
                  if paused == 1:
                     paused = 0
                  elif paused==0:
                     paused = 1
               elif paused==0:
                  playing=0
         elif topic == '/camera/odom/sample':
            #print (msg.pose)
            pass
            
      #close the bag file and close windows
      bag.close()
      cv2.destroyAllWindows()


def main(args):
   rospy.init_node('inference_node', anonymous=True)
   cv2.namedWindow('image',cv2.WINDOW_NORMAL)

   ic = image_inference()

   ic.read_bag(bag_name = args[1])
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
