#This file is for collecting goal tracking data from a ROS bag file.  The
#file must publish to /see3cam_cu20/image_raw/compressed and also must
#publish the odometry to /camera/odom/sample (T265 tracking camera)
#
#To run:  python collect_goal_images.py file.bag

import sys, cv2, time, queue, os, math, glob, random, shutil
import numpy as np
from threading import Lock

class image_tracker:
   def __init__(self):
      self.path = "./Input_Images"
      self.counter = 0
      self.filelist = [f for f in glob.glob(os.path.join(self.path,"*.jpg"))]
      self.filelist.sort()

      print(self.filelist)

      self.mouseX = 0
      self.mouseY = 0
      self.key = 0
      self.l = Lock()
      self.width_threshold = 200
      # self.width_threshold = 106

      cv2.namedWindow('image',cv2.WINDOW_NORMAL)
      cv2.setMouseCallback('image',self.mouse_click)

   def mouse_click(self,event,x,y,flags,param):
      if event == cv2.EVENT_LBUTTONDOWN:
         self.mouseX, self.mouseY = x,y
         self.target_set = 1

         print ((x,y))
         self.coord[-3] = str(self.mouseX)
         self.coord[-2] = str(self.mouseY)
         self.new_filename = '-'
         self.new_filename = self.new_filename.join(self.coord) + '.jpg' #combine the filename fields together
         print (self.new_filename)

         cv2.circle(self.img1, (int(self.coord[-3]), int(self.coord[-2])), 2, (0,255,255), -1)
         cv2.imshow('image',self.img1)

   def process_images(self):
      counter = 0

      for f in self.filelist:
         self.new_filename = f
         print (counter)
         self.img1 = cv2.imread(f)
         self.orig_img = cv2.imread(f)
         coord = f.split('.jpg')   #remove .jpg extension
         self.coord = coord[0].split('-')

         cv2.circle(self.img1, (int(self.coord[-3]), int(self.coord[-2])), 2, (0,255,255), -1)

         #draw vertical lines
         cv2.line(self.img1, (320-self.width_threshold, 0), (320-self.width_threshold, 359), (0, 255, 255), 1)
         cv2.line(self.img1, (320+self.width_threshold, 0), (320+self.width_threshold, 359), (0, 255, 255), 1)

         cv2.imshow('image',self.img1)

         while True:
            key = cv2.waitKey(0)

            if key == ord('q'):
               cv2.destroyAllWindows()
               return
            elif key == ord('n'):
               break
            elif key == ord('s'): #save the new file and delete the old file
               print("old file: " + f)
               print("new file: " + self.new_filename)
               os.remove(f)
               cv2.imwrite(self.new_filename,self.orig_img)
               break
            elif key == ord('c'): #set the goal to the upper center (no goal state)
               self.mouse_click(cv2.EVENT_LBUTTONDOWN,320,0,0,0)
            elif key == ord('a'): #skip the image
               break

         counter += 1

def copy_imgs():
   source_path = "./Input_Images"
   dest_train = "./Training_Images"
   dest_val = "./Validation_Images"

   source_filelist = [f for f in glob.glob(os.path.join(source_path,"*.jpg"))]
   random.shuffle(source_filelist)
   print(source_filelist)
   val_count = int(len(source_filelist)*.15)

   val_list = source_filelist[0:val_count] #build the list of validation images
   train_list = source_filelist[val_count:] #build the training images list
   print(len(val_list))
   print(len(train_list))

   for f in val_list:
      new_val_file = os.path.join(dest_val,os.path.basename(f))
      print(f, new_val_file)
      shutil.copy(f, new_val_file)

   for f in train_list:
      new_train_file = os.path.join(dest_train,os.path.basename(f))
      print(f, new_train_file)
      shutil.copy(f, new_train_file)

def main(args):
   if len(args) > 1:
      if args[1] == "dirs":
         copy_imgs()
         sys.exit()

   ic = image_tracker()
   ic.process_images()

   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
