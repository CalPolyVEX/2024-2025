#!/usr/bin/env python
#This script saves .jpg images from the /image_raw_throttle topic
#That topic typically publishes at 1Hz

#playback the .bag file
#rosrun image_transport republish compressed in:=/see3cam_cu20/image_raw raw out:=/see3cam_cu20/raw

from __future__ import print_function

import roslib
roslib.load_manifest('image_cnn')
import sys
import rospy
import cv2
import time, datetime
from std_msgs.msg import String
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError

class image_converter:

   def __init__(self, max=100):
      self.max = max
      self.counter = 0
      self.image_pub = rospy.Publisher("/see3cam_cu20/test_image_topic_2",Image,queue_size=1)

      self.bridge = CvBridge()
      self.image_sub = rospy.Subscriber("/zed_node/rgb/image_rect_color",Image,self.callback)
      self.last_time = time.time()

   def callback(self,data):
      try:
         cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
      except CvBridgeError as e:
         print(e)

      #save the file
      now = time.time()
      print(now)
      if (now - self.last_time) > 5.0:
         milliseconds = '%03d' % int((now - int(now)) * 1000)
         #timestamp = time.strftime('%H-%M-%S-%m-%d-%Y-', now) + milliseconds
         filename = str(datetime.datetime.now().replace(microsecond=0).isoformat())+'.jpg'

         print ('writing file ' + str(self.counter) + ': ' + filename)
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

         self.last_time = now
         if self.counter == self.max:
            self.image_sub.unregister()
            print ("Exiting...")
            sys.exit(0)
            rospy.signal_shutdown()

def main(args):
   print(args)
   if len(args) == 2:
     print(args)
     ic = image_converter(int(args[1]))
   else:
     ic = image_converter(100)

   rospy.init_node('image_converter', anonymous=True)
   try:
      rospy.spin()
   except KeyboardInterrupt:
      print("Shutting down")
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
