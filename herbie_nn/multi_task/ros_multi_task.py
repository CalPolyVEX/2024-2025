#!/usr/bin/env python3
from __future__ import print_function

import roslib
import sys, rospy, cv2, time
import numpy as np
from sensor_msgs.msg import CompressedImage
import torch
from torchvision import transforms
import math
from datetime import datetime

class image_inference:
   def __init__(self, model_name = "traced_efficientnetlite.pt", unlabeled = False):
      self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
      print (self.device)

      self.model = torch.load(model_name, map_location=self.device)
      self.model.eval()

      print ("Testing: ")
      print (self.model)

      if unlabeled == True:
         self.unlabelling = 1
      else:
         self.unlabelling = 0
      self.unlabeled_counter = 0

      self.image_resized_pub = rospy.Publisher("/test_image/image_raw/compressed",CompressedImage, queue_size=1)

      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw/compressed",CompressedImage,self.callback, queue_size=1, buff_size=10000000)

      self.preprocess = transforms.Compose([
       transforms.ToTensor(),
       #transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
       ])

      self.point_list = []

   def connect_boundary(self, points, img):
      for i in range(len(points)-1):
         x1= points[i][0]
         y1 = points[i][1]
         x2= points[i+1][0]
         y2 = points[i+1][1]
         cv2.line(img, (x1,y1), (x2,y2), (0,255,255), 2)

   def localization(self, output, img):
      font                   = cv2.FONT_HERSHEY_SIMPLEX
      lineType               = 2
      max_index = 0
      max_val = 0

      for c in range(64):
         val = float(output[1].data[0][c])
         if val > max_val:
            max_val = val
            max_index = c
      # self.straight_percentage = int(100 * float(output[1].data[0][64]))
      # self.turn_percentage = int(100 * float(output[1].data[0][65]))

      cv2.putText(img,str(max_index) + '  ' + "{:.3f}".format((max_val)),
          (430,50),
          font,
          .8,
          (255,0,255),
          lineType)

      # cv2.putText(image_np_360,str(self.straight_percentage) + '  ' + str(self.turn_percentage),
      #     (430,80),
      #     font,
      #     .8,
      #     (255,0,255),
      #     lineType)

   def callback(self,msg_in):
      #### direct conversion to CV2 ####
      # Convert it to something opencv understands
      start_time = time.time_ns()
      np_arr = np.frombuffer(msg_in.data, np.uint8)
      image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

      image_np = cv2.resize(image_np, (640, 360))  #resize
      image_np_360 = image_np

      image_np_tensor = self.preprocess(image_np)
      image_np_tensor = image_np_tensor.to(device=self.device)
      #image_np_tensor = transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])(image_np_tensor)
      image_np_tensor = image_np_tensor.unsqueeze(0)

      #run the forward pass of the model
      with torch.no_grad():
          output = self.model(image_np_tensor)
      end_time = time.time_ns()

      self.point_list = []
      for c in range(0,80):
          cv2.circle(image_np_360, ((c*8), round(float(output[0].data[0][c]) * (360-1))), 1, (0, 255, 0), -1)
          self.point_list.append((c*8,round(float(output[0].data[0][c]) * (360-1))))

      font                   = cv2.FONT_HERSHEY_SIMPLEX
      fontScale              = .7
      fontColor              = (255,255,255)
      lineType               = 2
      ms_time = (end_time - start_time)/1000000.0
      fps = 1000.0/ms_time

      self.connect_boundary(self.point_list,image_np_360)
      self.localization(output,image_np_360)

      cv2.putText(image_np_360,str(f'{ms_time:.2f}') + 'ms FPS: ' + str(fps),
          (430,25),
          font,
          fontScale,
          fontColor,
          lineType)

      #create the message to publish
      msg = CompressedImage()
      msg.header.stamp = rospy.Time.now()
      msg.format = "jpeg"
      msg.data = np.array(cv2.imencode('.jpg', image_np_360)[1]).tobytes()

      try:
         self.image_resized_pub.publish(msg)
      except CvBridgeError as e:
         print(e)

def main(args):
   rospy.init_node('inference_node', anonymous=True)

   if len(args) == 2 and args[1] == 'unlabeled':
      #if the argument is 'unlabeled', then record images periodically
      #this is for the ground detection neural network
       ic = image_inference(unlabeled = True)
   elif len(args) == 2:
       ic = image_inference(model_name = args[1], unlabeled = False)
   else:
       ic = image_inference()

   try:
      rospy.spin()
   except KeyboardInterrupt:
      print("Shutting down")
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
