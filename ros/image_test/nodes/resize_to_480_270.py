#!/usr/bin/env python
from __future__ import print_function

import roslib
roslib.load_manifest('image_test')
import sys, rospy, cv2, time, imp
import numpy as np
from std_msgs.msg import String
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError

class image_converter:
   def __init__(self):
      self.counter = 0
      self.image_resized_pub = rospy.Publisher("/see3cam_cu20/image_raw_480_270",Image, queue_size=1)

      self.bridge = CvBridge()
      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw",Image,self.callback)

   def callback(self,data):
      try:
         cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
      except CvBridgeError as e:
         print(e)

      #save the file
      (rows,cols,channels) = cv_image.shape

      #resize and conver the image to numpy array
      resized_image = cv2.resize(cv_image, (480, 270)) 

      try:
         self.image_resized_pub.publish(self.bridge.cv2_to_imgmsg(resized_image, "bgr8"))
      except CvBridgeError as e:
         print(e)

def main(args):
   ic = image_converter()
   rospy.init_node('infer_node', anonymous=True)
   try:
      rospy.spin()
   except KeyboardInterrupt:
      print("Shutting down")
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
