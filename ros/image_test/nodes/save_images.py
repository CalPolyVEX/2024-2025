#!/usr/bin/env python
#This script saves .jpg images from the /image_raw_throttle topic
#That topic typically publishes at 1Hz

from __future__ import print_function

import roslib
roslib.load_manifest('image_test')
import sys
import rospy
import cv2
import time
from std_msgs.msg import String
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError

class image_converter:

   def __init__(self):
      self.counter = 0
      self.image_pub = rospy.Publisher("/see3cam_cu20/test_image_topic_2",Image)

      self.bridge = CvBridge()
      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw_throttle",Image,self.callback)

   def callback(self,data):
      try:
         cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
      except CvBridgeError as e:
         print(e)


      #save the file
      now = time.time()
      localtime = time.localtime(now)
      milliseconds = '%03d' % int((now - int(now)) * 1000)
      timestamp = time.strftime('%H-%M-%S-%m-%d-%Y-', localtime) + milliseconds
      filename = str(timestamp)+'.jpg'

      print ('writing file (' + self.counter + ': ' + filename)
      self.counter += 1
      cv2.imwrite(filename, cv_image)

      (rows,cols,channels) = cv_image.shape
      if cols > 60 and rows > 60 :
         cv2.circle(cv_image, (50,50), 20, 255)

      #cv2.imshow("Image window", cv_image)
      #cv2.waitKey(3)

      try:
         self.image_pub.publish(self.bridge.cv2_to_imgmsg(cv_image, "bgr8"))
      except CvBridgeError as e:
         print(e)

def main(args):
   ic = image_converter()
   rospy.init_node('image_converter', anonymous=True)
   try:
      rospy.spin()
   except KeyboardInterrupt:
      print("Shutting down")
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
