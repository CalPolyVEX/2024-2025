#!/usr/bin/env python3
from __future__ import print_function

import roslib, rosbag
import sys, rospy, cv2, time, queue
import numpy as np
from sensor_msgs.msg import CompressedImage
import torch
from torchvision import transforms
import math

class image_tracker:
   def __init__(self): 
      self.tracker = cv2.TrackerKCF_create()

      self.image_np = None
      self.counter = 0
      self.tracking = 0

      self.mouseX = 0
      self.mouseY = 0
      self.target_set = 0
      self.key = 0
      self.box_width = 100
      self.last_yaw = 0
      self.yaw_list = [0,0,0,0,0,0,0]
      self.heading_offset = 0
      self.last_heading_offset = 0

      #how many frames to skip before capturing an image
      self.straight_capture_freq = 20

      #how many frames to skip in a turn before capturing an image
      self.turn_capture_freq = 3 #this number is multiplier of how 
      self.capture_freq = self.straight_capture_freq

      self.straight_capture_count = 0
      self.turn_capture_count = 0

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
         # print (self.counter)
         center_x = x+int(self.box_width/2)
         center_y = y+int(self.box_width/2)
         if self.counter % self.capture_freq == 0:
            self.straight_capture_count += 1
            print (self.straight_capture_count)
            cv2.imwrite(self.file_prefix + '-' + str(self.straight_capture_count) + '-' + str(center_x) + '-' + str(center_y) + '.jpg', self.image_orig)


   def read_bag(self, bag_name = '/home/jseng/ref_bags/2021-03-23-08-06-14.bag'):
      bag = rosbag.Bag(bag_name)
      cv2.setMouseCallback('image',self.mouse_click)
      self.file_prefix = bag_name.split('/')[-1]
      self.file_prefix = self.file_prefix.split('.bag')[0]
      print (self.file_prefix)

      for topic, msg, t in bag.read_messages(topics=['/see3cam_cu20/image_raw/compressed', '/camera/odom/sample']):
         if topic == '/see3cam_cu20/image_raw/compressed':
            np_arr = np.frombuffer(msg.data, np.uint8)
            self.image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
            self.image_np = cv2.resize(self.image_np, (640, 360))  #resize
            self.image_orig = self.image_np.copy() #the data to save

            #waiting for first click
            if self.counter == 0:
               print ("Waiting for first click")
               cv2.imshow('image',self.image_np)
               while self.target_set == 0:
                  cv2.waitKey(1)

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
                  center_x = x+int(w/2)
                  center_y = y+int(h/2)
                  cv2.circle(self.image_np, (center_x,center_y), 4, (0,255,255), -1)
                  cv2.imshow('image',self.image_np)

                  #save image data here
                  self.counter += 1
                  #print (self.counter)
                  self.image_orig_jpg = cv2.imencode('.jpg', self.image_orig)
                  if self.counter % self.capture_freq == 0:
                     self.straight_capture_count += 1
                     print (self.straight_capture_count)
                     cv2.imwrite(self.file_prefix + '-' + str(self.straight_capture_count) + '-' + str(center_x) + '-' + str(center_y) + '.jpg', self.image_orig)
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
            w = msg.pose.pose.orientation.w
            x = msg.pose.pose.orientation.x
            y = msg.pose.pose.orientation.y
            z = msg.pose.pose.orientation.z
            t3 = +2.0 * (w * z + x * y)
            t4 = +1.0 - 2.0 * (y * y + z * z)
            yaw = 57.2958 * math.atan2(t3, t4)

            #this code handles the wrap-around of the heading from +180 to -180
            if self.last_yaw > 175 and yaw < -175:
                self.heading_offset += 360.0
            elif self.last_yaw < -175 and yaw > 175:
                self.heading_offset -= 360.0

            #compute a running average
            for i in range(len(self.yaw_list)-1):
               self.yaw_list[i] = self.yaw_list[i+1]
            self.yaw_list[-1] = (self.last_heading_offset + self.last_yaw) - (self.heading_offset + yaw)
            yaw_average = sum(self.yaw_list) / len(self.yaw_list)

            # print (self.yaw_list)
            # print (yaw_average)

            yaw_threshold = .09
            if yaw_average > yaw_threshold:
               #print ("turning right")
               self.capture_freq = self.turn_capture_freq
            elif yaw_average < -yaw_threshold:
               #print ("turning left")
               self.capture_freq = self.turn_capture_freq
            else:
               self.capture_freq = self.straight_capture_freq

            self.last_yaw = yaw
            self.last_heading_offset = self.heading_offset
            
      #close the bag file and close windows
      bag.close()
      cv2.destroyAllWindows()


def main(args):
   rospy.init_node('inference_node', anonymous=True)
   cv2.namedWindow('image',cv2.WINDOW_NORMAL)

   ic = image_tracker()

   ic.read_bag(bag_name = args[1])
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
