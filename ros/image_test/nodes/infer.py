#!/usr/bin/env python
from __future__ import print_function

import roslib
roslib.load_manifest('image_test')
import sys, rospy, cv2, time, imp
import numpy as np
import tensorflow as tf
from std_msgs.msg import String
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError

class image_converter:
   def __init__(self, infer_file):
      self.foo = imp.load_source('module.name', infer_file)
      self.input_layer='conv2d_1_input'
      self.input_layer='input_1'
      self.output_layer='k2tfout_0'
      self.gd = self.foo.GroundDetector('./output_graph.pb', self.input_layer, self.output_layer)

      self.counter = 0
      self.image_pub = rospy.Publisher("/see3cam_cu20/test_image_topic_3",Image, queue_size=1)
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

      #resize and convert the image to numpy array
      resized_image_nn = cv2.resize(cv_image, (480, 270)) 
      resized_image = resized_image_nn.copy() 
      np_image_data = np.asarray(resized_image_nn)
      #float_caster = tf.cast(np_image_data, tf.float32)
      float_caster = np_image_data / 255.0
      np_final = np.expand_dims(float_caster,axis=0)
      #print (np_final.shape)

      time1 = time.time()
      results = self.gd.run(np_final)
      time2 = time.time()
      results *= 270.0

      column = 5

      font = cv2.FONT_HERSHEY_SIMPLEX
      cv2.putText(resized_image_nn, "%.2fms" % ((time2-time1)*1000.0), (390, 20), font, 0.6, (0, 255, 0), 1, cv2.LINE_AA)
      for x in results:
         cv2.circle(resized_image_nn, (int(column),int(x)), 2, (0,0,255), 3)
         column += 10

      try:
         self.image_pub.publish(self.bridge.cv2_to_imgmsg(resized_image_nn, "bgr8"))
         self.image_resized_pub.publish(self.bridge.cv2_to_imgmsg(resized_image, "bgr8"))
      except CvBridgeError as e:
         print(e)

def main(args):
   ic = image_converter(args[1])
   rospy.init_node('infer_node', anonymous=True)
   try:
      rospy.spin()
   except KeyboardInterrupt:
      print("Shutting down")
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
