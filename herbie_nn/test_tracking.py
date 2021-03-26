#!/usr/bin/env python3
from __future__ import print_function

import roslib
import sys, rospy, cv2, time, queue
import numpy as np
from sensor_msgs.msg import CompressedImage
import torch
from torchvision import transforms
import math

mouseX = 0
mouseY = 0
key = 0

def mouse_click(event,x,y,flags,param):
   global mouseX,mouseY
   if event == cv2.EVENT_LBUTTONDOWN:
      mouseX,mouseY = x,y
      print ((x,y))

class image_inference:
   global q

   def __init__(self, model_name = "mobilenetv2_100-.010-80out.pt"):
      #create the window

      self.image_resized_pub = rospy.Publisher("/test_image/image_raw/compressed",CompressedImage, queue_size=1)
      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw/compressed",CompressedImage,self.callback, queue_size=1, buff_size=10000000)
      self.tracker = cv2.TrackerKCF_create()

      self.point_list = []
      self.image_np_360 = None
      self.counter = 0


   def callback(self,msg_in):
      global mouseX, mouseY, key

      #### direct conversion to CV2 ####
      # Convert it to something opencv understands
      start_time = time.time_ns()
      np_arr = np.frombuffer(msg_in.data, np.uint8)
      image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

      image_np = cv2.resize(image_np, (640, 360))  #resize
      image_np_360 = image_np
      image_np = image_np[:,:,::-1].copy() # convert BGR to RGB

      self.counter += 1
      # if self.counter % 30 == 0:
      #    ball = image_np_360[310:340, 330:350]
      #    self.tracker.init(image_np_360, ball)

      cv2.imshow('image',image_np_360)
      cv2.setMouseCallback('image',mouse_click)
      key = cv2.waitKey(50) 

      if key != -1:
         print (mouseX,mouseY)


      # cv2.putText(image_np_360,str(f'{ms_time:.2f}') + 'ms FPS: ' + str(fps),
      #     (430,25),
      #     font,
      #     fontScale,
      #     fontColor,
      #     lineType)

def main(args):
   rospy.init_node('inference_node', anonymous=True)
   #cv2.namedWindow('image',cv2.WINDOW_NORMAL)
   #cv2.setMouseCallba\\wck('image',mouse_click)
   r = rospy.Rate(200) # 200Hz

   if len(args) == 2:
       ic = image_inference(model_name = args[1])
   else:
       ic = image_inference()

   while not rospy.is_shutdown(): 
      if key == ord('q'):
         break
      r.sleep()

   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
