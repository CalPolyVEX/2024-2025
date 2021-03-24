#!/usr/bin/env python3
from __future__ import print_function

import roslib
import sys, rospy, cv2, time
import numpy as np
from sensor_msgs.msg import CompressedImage
import torch
from torchvision import transforms
import math

class image_inference:
   def __init__(self, model_name = "mobilenetv2_100-.010-80out.pt"):
      self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
      print (self.device)

      self.model = torch.load(model_name, map_location=self.device)
      #self.model = torch.load("/home/jseng/Senior-Project/pytorch_networks/mobilenetv3_small-.0110-80out.pt", map_location=torch.device('cpu'))
      self.model.eval()

      self.image_resized_pub = rospy.Publisher("/test_image/image_raw/compressed",CompressedImage, queue_size=1)
      #self.image_resized_pub = rospy.Publisher("/test_image/image_raw",Image, queue_size=1)

      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw/compressed",CompressedImage,self.callback, queue_size=1, buff_size=10000000)

      self.preprocess = transforms.Compose([
       transforms.ToTensor(),
       #transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
       ])

      self.point_list = []

   def local_planner(self, img):
      #print (self.point_list)
      middle_x = 320
      middle_y = 180

      distance_list = []

      #remove the items at the bottom of the image starting at the left
      i = 40
      found = 0
      while i >= 0:
        if self.point_list[i][1] >= 359:
           self.point_list[i] = (-1,-1)
           found = 1
        if found == 1:
           self.point_list[i] = (-1,-1)
        i -= 1

      #remove the items at the bottom of the image starting at the right
      i = 40
      found = 0
      while i < len(self.point_list):
         if self.point_list[i][1] >= 359:
            self.point_list[i] = (-1,-1)
            found = 1
         if found == 1:
            self.point_list[i] = (-1,-1)
         i += 1

      print ('--------------')
      print (self.point_list)

      #clear out all element with -1,-1
      temp_distance_list = []
      for i in range(len(self.point_list)):
         if self.point_list[i][0] != -1:
            temp_distance_list.append(self.point_list[i])

      distance_list = temp_distance_list
      print (distance_list)

      for p in distance_list:
         px = p[0]
         py = p[1]
         #distance = math.sqrt((middle_x - px)**2 + (middle_y - py)**2)
         #distance_list.append(distance)
         cv2.line(img, (px,py), (middle_x,359), (255,0,0), 2)

      print (len(distance_list))

   def callback(self,msg_in):
      #### direct conversion to CV2 ####
      # Convert it to something opencv understands
      start_time = time.time_ns()
      np_arr = np.frombuffer(msg_in.data, np.uint8)
      image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

      image_np = cv2.resize(image_np, (640, 360))  #resize
      image_np_360 = image_np
      image_np = image_np[:,:,::-1].copy() # convert BGR to RGB

      image_np_tensor = self.preprocess(image_np)
      image_np_tensor = image_np_tensor.to(device=self.device)
      image_np_tensor = transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])(image_np_tensor)
      image_np_tensor = image_np_tensor.unsqueeze(0)

      #run the forward pass of the model
      with torch.no_grad():
          output = self.model(image_np_tensor)
      end_time = time.time_ns()

      #print (output)
      #print (output.data[0])

      self.point_list = []
      for c in range(0,80):
          cv2.circle(image_np_360, ((c*8), round(float(output.data[0][c]) * (360-1))), 1, (0, 255, 0), -1)
          self.point_list.append((c*8,round(float(output.data[0][c]) * (360-1))))

      font                   = cv2.FONT_HERSHEY_SIMPLEX
      fontScale              = .7
      fontColor              = (255,255,255)
      lineType               = 2
      ms_time = (end_time - start_time)/1000000.0
      fps = 1000.0/ms_time

      self.local_planner(image_np_360)

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
   if len(args) == 2:
       ic = image_inference(model_name = args[1])
   else:
       ic = image_inference()

   try:
      rospy.spin()
   except KeyboardInterrupt:
      print("Shutting down")
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
