#This file is for collecting goal tracking data from a ROS bag file.  The
#file must publish to /see3cam_cu20/image_raw/compressed and also must
#publish the odometry to /camera/odom/sample (T265 tracking camera)
#
#To run:  python collect_goal_images.py file.bag

import roslib, rosbag
import sys, rospy, cv2, time, queue, os, math
import numpy as np
from sensor_msgs.msg import CompressedImage
from threading import Lock

class image_tracker:
   def __init__(self):
      #self.tracker = cv2.TrackerKCF_create()
      self.tracker = cv2.TrackerCSRT_create()

      self.image_np = None
      self.counter = 0
      self.tracking = 0

      self.mouseX = 0
      self.mouseY = 0
      self.target_set = 0
      self.key = 0
      self.box_width = 100
      self.last_heading = 0
      self.heading_list = [0] * 30
      self.heading_offset = 0
      self.last_heading_offset = 0
      self.turn_dir = 2 # 1 is left, 2 is straight, 3 is right
      self.l = Lock()

      #how many frames to skip before capturing an image
      self.straight_capture_freq = 20

      #how many frames to skip in a turn before capturing an image
      self.turn_capture_freq = 3 #this number is multiplier of how
      self.capture_freq = self.straight_capture_freq

      self.straight_capture_count = 0
      self.turn_capture_count = 0

      #create Input_Images directory if it does not exist
      if not os.path.exists('./Input_Images'):
          os.makedirs('./Input_Images')

   def mouse_click(self,event,x,y,flags,param):
      if event == cv2.EVENT_LBUTTONDOWN:
         self.mouseX, self.mouseY = x,y
         self.target_set = 1

         #reinitialize the tracker on a mouse click
         #self.tracker = cv2.TrackerKCF_create()
         self.tracker = cv2.TrackerCSRT_create()

         bbox = (self.mouseX- int(self.box_width/2), self.mouseY - int(self.box_width/2), self.box_width, self.box_width)

         self.tracker.init(self.image_np, bbox)
         self.tracking = 1
         print ((x,y))
         cv2.circle(self.image_np, (x,y), 4, (0,255,255), -1)
         cv2.imshow('image',self.image_np)

         #save image data here
         if self.l.locked() == False:
            self.counter += 1
            # print (self.counter)
            center_x = self.mouseX
            center_y = self.mouseY

            if not ((center_x > 320 and self.turn_dir == 1) or (center_x < 320 and self.turn_dir == 3)):
               self.straight_capture_count += 1
               print (self.straight_capture_count)
               #cv2.circle(self.image_orig, (center_x,center_y), 4, (0,255,255), -1)
               # cv2.imwrite('./Input_Images/' + self.file_prefix + '-' + str(self.turn_dir) + \
               #    '-' + str(center_x) + '-' + str(center_y) + '-' + \
               #    str(self.straight_capture_count) + '.jpg', \
               #    self.image_orig)

   def read_bag(self, bag_name = '/home/jseng/ref_bags/2021-03-23-08-06-14.bag'):
      bag = rosbag.Bag(bag_name)
      cv2.setMouseCallback('image',self.mouse_click)
      self.file_prefix = bag_name.split('/')[-1]
      self.file_prefix = self.file_prefix.split('.bag')[0]
      print (self.file_prefix)

      for topic, msg, t in bag.read_messages(topics=['/see3cam_cu20/image_raw/compressed', '/camera/odom/sample']):
         if topic == '/see3cam_cu20/image_raw/compressed':
            np_arr = np.frombuffer(msg.data, np.uint8)
            self.image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR) #decode the JPG
            self.image_np = cv2.resize(self.image_np, (640, 360))  #resize
            self.image_orig = self.image_np.copy() #the data to save

            #waiting for first click
            if self.counter == 0:
               print ("Waiting for first click.  Click on the image to reposition the tracking target.  Use the \'z\' key to pause")
               cv2.imshow('image',self.image_np)
               while self.target_set == 0:
                  cv2.waitKey(1)

            if self.tracking == 0:
               #self.tracker = cv2.TrackerKCF_create()
               self.tracker = cv2.TrackerCSRT_create()

               bbox = (self.mouseX - int(self.box_width/2), self.mouseY - \
                  int(self.box_width/2), self.box_width, self.box_width)

               #initialize tracker
               self.tracker.init(self.image_np, bbox)
               self.tracking = 1
               cv2.circle(self.image_np, (self.mouseX-int(self.box_width/2), \
                  self.mouseY-int(self.box_width/2)), 4, (0,255,255), -1)
               cv2.imshow('image',self.image_np)
            else:
               (success, box) = self.tracker.update(self.image_np)
               # check to see if the tracking was a success
               if success:
                  (x, y, w, h) = [int(v) for v in box]
                  cv2.rectangle(self.image_np, (x, y), (x + w, y + h), (0, 255, 0), 2)
                  self.tracking = 1
                  center_x = x+int(w/2)
                  center_y = y+int(h/2)
                  cv2.circle(self.image_np, (center_x,center_y), 4, (0,255,255), -1)
                  cv2.imshow('image',self.image_np)

                  #save image data here
                  self.l.acquire()
                  self.counter += 1
                  #print (self.counter)
                  if self.counter % self.capture_freq == 0:
                     #check for invalid conditions

                     if not ((center_x > 320 and self.turn_dir == 1) or \
                         (center_x < 320 and self.turn_dir == 3)):
                        self.straight_capture_count += 1
                        print (self.straight_capture_count)
                        #cv2.circle(self.image_orig, (center_x,center_y), 4, (0,255,255), -1)
                        # cv2.imwrite('./Input_Images/' + self.file_prefix + '-' + str(self.turn_dir) + \
                        #    '-' + str(center_x) + '-' + str(center_y) + '-' + \
                        #    str(self.straight_capture_count) + '.jpg', \
                        #    self.image_orig)
                  self.l.release()
               else:
                  print ('tracking lost')
                  self.tracking = 0
                  self.target_set = 0
                  while self.target_set == 0:
                     cv2.waitKey(1)

            #hit 'q' to quit, 'z' to pause
            playing = 1
            paused = 0

            while playing == 1 or paused == 1:
               key = cv2.waitKey(50)
               if key == ord('q'):
                  bag.close()
                  cv2.destroyAllWindows()
                  return
               elif key == ord('z'):
                  if paused == 1:
                     paused = 0
                  elif paused==0:
                     paused = 1
               elif paused==0:
                  playing=0
         elif topic == '/camera/odom/sample':
            #convert the quaternion to a heading
            w = msg.pose.pose.orientation.w
            x = msg.pose.pose.orientation.x
            y = msg.pose.pose.orientation.y
            z = msg.pose.pose.orientation.z
            t3 = +2.0 * (w * z + x * y)
            t4 = +1.0 - 2.0 * (y * y + z * z)
            heading = 57.2958 * math.atan2(t3, t4)

            #this code handles the wrap-around of the heading from +180 to -180
            if self.last_heading > 175 and heading < -175:
                self.heading_offset += 360.0
            elif self.last_heading < -175 and heading > 175:
                self.heading_offset -= 360.0

            #compute a running average
            for i in range(len(self.heading_list)-1):
               self.heading_list[i] = self.heading_list[i+1]
            self.heading_list[-1] = (self.last_heading_offset + self.last_heading) - \
               (self.heading_offset + heading)
            heading_average = sum(self.heading_list) / len(self.heading_list)

            # print (self.heading_list)
            # print (heading_average)

            heading_threshold = .10
            if heading_average > heading_threshold: #turning right
               print ("turning right")
               self.capture_freq = self.turn_capture_freq
               self.turn_dir = 3
            elif heading_average < -heading_threshold: #turning left
               print ("turning left")
               self.capture_freq = self.turn_capture_freq
               self.turn_dir = 1
            else: #going straight
               self.capture_freq = self.straight_capture_freq
               self.turn_dir = 2

            self.last_heading = heading
            self.last_heading_offset = self.heading_offset

      #close the bag file and close windows
      bag.close()
      cv2.destroyAllWindows()


def main(args):
   rospy.init_node('tracking_node', anonymous=True)
   cv2.namedWindow('image',cv2.WINDOW_NORMAL)

   ic = image_tracker()

   ic.read_bag(bag_name = args[1])
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
