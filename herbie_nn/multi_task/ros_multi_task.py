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
from pretrained_model import Pretrained_Model
from std_msgs.msg import Float64MultiArray

class image_inference:
   def __init__(self, model_name = "traced_efficientnetlite.pt", unlabeled = False):
      self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
      print (self.device)

      # load the model
      self.model = torch.load(model_name, map_location=self.device)
      self.model.eval()

      print ("Testing: ")
      print (self.model)

      self.image_resized_pub = rospy.Publisher("/test_image/image_raw/compressed",CompressedImage, queue_size=1)

      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw/compressed",CompressedImage,self.callback, queue_size=1, buff_size=10000000)

      self.data_pub = rospy.Publisher('/nn_data', Float64MultiArray, queue_size=1)

      self.preprocess = transforms.Compose([
       transforms.ToTensor(),
       ])

      self.point_list = []
      self.goal_list_x = [0] * 5
      self.goal_list_y = [0] * 5

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
      second_index = 0
      second_val = 0

      loc_list = []
      loc_counter = 0
      for c in range(64):
         val = float(output[1].data[0][c])
         if val > .2:
             loc_list.append(val)
             loc_counter += 1

         if val > max_val:
            max_val = val
            max_index = c

    #   loc_var = np.var(loc_list)

      straight_percent = output[2].data[0][0]
      turn_percent = output[2].data[0][1]

      # self.straight_percentage = int(100 * float(output[1].data[0][64]))
      # self.turn_percentage = int(100 * float(output[1].data[0][65]))

      cv2.putText(img,str(max_index) + '  ' + "{:.3f}".format((max_val)),
          (430,50),
          font,
          .8,
          (255,0,255),
          lineType)

    #   cv2.putText(img,'straight: ' + "{:.3f}".format(straight_percent),
    #       (430,80),
    #       font,
    #       .8,
    #       (255,0,255),
    #       lineType)

    #   cv2.putText(img,'turn: ' + "{:.3f}".format(turn_percent),
    #       (430,110),
    #       font,
    #       .8,
    #       (255,0,255),
    #       lineType)

      cv2.putText(img,'counter: ' + "{:.3f}".format(loc_counter),
          (430,140),
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

   def navigation_lines(self,img):
       # 3 foot lines in front of robot
       cv2.line(img, (212,359), (256,229), (0,255,255), 2) # left trapezoid side
       cv2.line(img, (428,359), (384,229), (0,255,255), 2) # right trapezoid side

       # 2 foot lines in front of robot
       cv2.line(img, (212,359), (241,275), (255,0,0), 4) # left trapezoid side
       cv2.line(img, (428,359), (399,275), (255,0,0), 4) # right trapezoid side

       cv2.line(img, (320,0), (320,359), (255,255,0), 1) # vertical line down middle

   def draw_goal(self, img, output):
       center_x = float(output[3].data[0][0])
       center_y = float(output[3].data[0][1])

       self.goal_list_x.append(center_x)
       self.goal_list_y.append(center_y)
       self.goal_list_x.pop(0)
       self.goal_list_y.pop(0)

       var_x = np.var(self.goal_list_x)
       var_y = np.var(self.goal_list_y)

       #print (var_x, var_y)

       if var_x < .0009 and var_y < .0009:
           avg_x = np.median(self.goal_list_x)
           avg_y = np.median(self.goal_list_y)
           cv2.circle(img, (int(avg_x*640), int(avg_y*360)), 3, (0,255,0), -1)
       else:
           cv2.circle(img, (int(center_x*640), int(center_y*360)), 3, (255,0,255), -1)

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
      image_np_tensor = image_np_tensor.unsqueeze(0)

      #run the forward pass of the model
      with torch.no_grad():
          output = self.model(image_np_tensor)
      end_time = time.time_ns()

      #draw the ground boundary
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
      self.navigation_lines(image_np_360)

      # draw the goal circle
      self.draw_goal(image_np_360, output)

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

      #create the data message
      data_to_send = Float64MultiArray()  # the data to be sent
      nn_data_list = []

      for p in range(80): # ground boundary data
          nn_data_list.append(float(output[0].data[0][p]))
      for p in range(64): # localization data
          nn_data_list.append(float(output[1].data[0][p]))
      for p in range(2): # goal data
          nn_data_list.append(float(output[3].data[0][p]))

      data_to_send.data = nn_data_list

      self.data_pub.publish(data_to_send)

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
