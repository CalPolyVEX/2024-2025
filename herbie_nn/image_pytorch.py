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
import torch
from torchvision import transforms
import time

class image_converter:
   def __init__(self):
      self.model = torch.load("mobilenetv2_100-.010-80out.pt", map_location=torch.device('cpu'))
      self.model.eval()

      self.counter = 0
      self.image_resized_pub = rospy.Publisher("/test_image/image_raw/compressed",CompressedImage, queue_size=1)
      #self.image_resized_pub = rospy.Publisher("/test_image/image_raw",Image, queue_size=1)

      self.bridge = CvBridge()
      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw/compressed",CompressedImage,self.callback, queue_size=1, buff_size=10000000)

      self.preprocess = transforms.Compose([
       transforms.ToTensor(),
       transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
       ])

   def callback(self,data):
      #### direct conversion to CV2 ####
      # Convert it to something opencv understands
      np_arr = np.frombuffer(data.data, np.uint8)
      image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

      image_np = cv2.resize(image_np, (640, 360))  #resize
      image_np_360 = image_np
      image_np = image_np[:,:,::-1].copy() # convert BGR to RGB

      image_np_tensor = self.preprocess(image_np)
      image_np_tensor = image_np_tensor.unsqueeze(0)

      #run the forward pass of the model
      with torch.no_grad():
          output = self.model(image_np_tensor)

      #print (output)
      #print (output.data[0])

      for c in range(0,80):
          cv2.circle(image_np_360, ((c*8), round(float(output.data[0][c]) * (360-1))), 1, (0, 255, 0), -1)

      #create the message to publish
      msg = CompressedImage()
      msg.header.stamp = rospy.Time.now()
      msg.format = "jpeg"
      msg.data = np.array(cv2.imencode('.jpg', image_np_360)[1]).tobytes()
      #msg.data = np.array(image_np_360).tobytes()

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
