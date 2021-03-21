#!/usr/bin/env python3
from __future__ import print_function

import roslib
#roslib.load_manifest('image_test')
import sys, rospy, cv2, time
import numpy as np
from std_msgs.msg import String
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError
from sensor_msgs.msg import CompressedImage

class image_converter:
   def __init__(self):
      self.counter = 0
      #self.image_resized_pub = rospy.Publisher("/see3cam_cu20/image_raw_480_270",Image, queue_size=1)
      self.image_resized_pub = rospy.Publisher("/see3cam_cu20/image_raw_480_270/compressed",CompressedImage, queue_size=1)

      self.bridge = CvBridge()
      #self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw/compressed",CompressedImage,self.callback)
      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw",Image,self.callback)

   def callback(self,data):
      try:
          # Convert your ROS Image message to OpenCV2
          cv2_img = self.bridge.imgmsg_to_cv2(data, "bgr8")
      except CvBridgeError as e:
          print(e)

      #### direct conversion to CV2 ####
      # Convert it to something opencv understands
      #np_arr = np.fromstring(data.data, np.uint8)
      #image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
      #image_np = cv2.imread(np_arr, cv2.IMREAD_COLOR)
      image_np = cv2.resize(cv2_img, (960, 540)) 

      msg = CompressedImage()
      msg.header.stamp = rospy.Time.now()
      msg.format = "jpeg"
      msg.data = np.array(cv2.imencode('.jpg', image_np)[1]).tostring()

      try:
         #self.image_resized_pub.publish(self.bridge.cv2_to_imgmsg(resized_image, "bgr8"))
         self.image_resized_pub.publish(msg)
      except CvBridgeError as e:
         print(e)

def main(args):
   rospy.init_node('infer_node', anonymous=True)
   ic = image_converter()
   try:
      rospy.spin()
   except KeyboardInterrupt:
      print("Shutting down")
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
