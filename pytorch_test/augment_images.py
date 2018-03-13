#!usr/bin/python

#usage ./label_me_polygons.py directory_of_files
#directory_of_files/Images
#directory_of_files/Annotations

import os, sys, random
from os import path
import cv2, numpy as np
import xml.etree.ElementTree
import Augmentor

if len(sys.argv) <= 1:
   print "need command line arguments"
   #sys.exit()
elif sys.argv[1] == 'clean':
   print "clean"
   os.system('cd ' + sys.argv[2] + '/320_input; rm *.jpg')
   os.system('cd ' + sys.argv[2] + '/320_data; rm *.txt')
   os.system('cd ' + sys.argv[2] + '/320_ground_truth; rm *.jpg')
   os.system('cd ' + sys.argv[2] + '/320_inference_input; rm *.jpg')
   os.system('cd ' + sys.argv[2] + '/320_final_inference_output; rm *.jpg')
   os.system('cd ' + sys.argv[2] + '/input; rm *.jpg')
   os.system('cd ' + sys.argv[2] + '/input_annotation; rm *.xml')
   os.system('cd ' + sys.argv[2] + '/ground_output; rm *.jpg')
   os.system('cd ' + sys.argv[2] + '/augmented_output; rm *.jpg')
   sys.exit()

class ImageAugmentor:
   def __init__(self):
      self.jpg_files = [] #a list of the original .jpg files
      self.input_files = [] #list of the files renamed: 0.jpg, 1.jpg, ...
      self.input_dir = path.join(sys.argv[1], "input")
      self.remove_image_with_no_polygon = 1
      self.xml_dir = path.join(sys.argv[1],"Annotations/users/jseng/building14")
      self.ground_output_dir = path.join(sys.argv[1],"ground_output")
   
   def fill_polygon(img):
      height, width, channels = img.shape

      #build a mask that is 2 pixels wider and taller

      mask=np.zeros((height+2, width+2), np.uint8)
      cv2.floodFill(img, mask, (0,0), (255,255,255))

      #run down the left side of the image and if there is a 
      #black pixel, then do a floodfill
      for y in range(height):
         x=0
         if img[y,x][2] != 255:
            cv2.floodFill(img, mask, (x,y), 255)

      for y in range(height):
         x=width-1
         if img[y,x][2] != 255:
            cv2.floodFill(img, mask, (x,y), 255)

      #invert the image
      img = cv2.bitwise_not(img)

      return img

   def crop_images(jpg_dir, jpg_files, output_dir):
      ca_list = [15,30,45]

      for crop_amount in ca_list:
         for f in jpg_files:
            print "cropping " + str(crop_amount) + " " + f
            img = cv2.imread(jpg_dir + '/' + f)
            gt_img = cv2.imread(ground_output_dir + '/gt_' + f)
            height, width, channels = img.shape

            crop_left_img = img[0:height-1, crop_amount:width-1] #trim left edge
            crop_gt_left_img = gt_img[0:height-1, crop_amount:width-1] #trim left edge
            
            crop_left_img = cv2.resize(crop_left_img, (width,height), interpolation=cv2.INTER_CUBIC)
            crop_gt_left_img = cv2.resize(crop_gt_left_img, (width,height), interpolation=cv2.INTER_CUBIC)
            cv2.imwrite(output_dir + '/crop_left' + str(crop_amount) + '_' + f, crop_left_img)
            cv2.imwrite(ground_output_dir + '/gt_crop_left' + str(crop_amount) + '_' + f, crop_gt_left_img)


   def mirror_images(jpg_dir, jpg_files, output_dir):
      for f in jpg_files:
         print "mirroring " + f
         img = cv2.imread(jpg_dir + '/' + f)
         gt_img = cv2.imread(ground_output_dir + '/gt_' + f)
         height, width, channels = img.shape


   #create the actual data to feed to neural network
   def get_range_data(dirs, out):
      print "Generating data files for images"

      if not os.path.exists(out):
         os.makedirs(out)

      for d in dirs:
         files = os.listdir(d)
         files.sort()
         for f in files:
            if 'lidar' in f:
               continue
            print f
            img = cv2.imread(d + '/' + f)
            height, width, channels = img.shape
            step = width / 31
            #blank_image = np.zeros((height,width,3), np.uint8)

            data=[]
            f_out = open(out + '/' + f.replace('.jpg', '.txt'), 'w')

            #run from 0 to the right edge of the image
            for x in range(5,width,step):
               temp = height-1  #start at the bottom of the image
               while temp >= 0:
                  pixel = img[temp,x]
                  if pixel[0] == 0 or temp == 0: #if the pixel is black or reach the top of image
                        #sys.stdout.write('%d, ' % temp)
                        #cv2.circle(blank_image,(x,temp),5,(0,0,255),-1)
                        data.append((x,temp))
                        f_out.write(str(x) + ',' + str(temp) + '\n')
                        #blank_image[temp,x] = [0,0,255]
                        found=1
                        break
                  temp = temp - 1

            f_out.close()
            print data
            #cv2.imwrite(out + '/lidar_' + f, blank_image)
      
   def build_320_240_images(in_dir):
      print "Converting input images to 320x240"
      files = os.listdir(in_dir)
      files.sort()

      #create directory for 320x240 input files
      input_dir_320 = sys.argv[1]+"/320_input"
      if not os.path.exists(input_dir_320):
         os.makedirs(input_dir_320)

      for f in files:  #resize all the input files
         img = cv2.imread(in_dir + '/' + f)
         img320 = cv2.resize(img,(320,240), interpolation=cv2.INTER_CUBIC)
         new_name = '320_' + f
         cv2.imwrite(input_dir_320 + '/' + new_name, img320)

   def build_320_240_gt_images(gt_dir):
      print "Converting ground truth images to 320x240"
      gt_files = os.listdir(gt_dir)
      gt_files.sort()

      #create directory for 320x240 ground truth files
      gt_dir_320 = sys.argv[1]+"/320_ground_truth"
      if not os.path.exists(gt_dir_320):
         os.makedirs(gt_dir_320)

      for f in gt_files:  #resize all the ground_truth files
         img = cv2.imread(gt_dir + '/' + f)
         img320 = cv2.resize(img,(320,240), interpolation=cv2.INTER_CUBIC)
         new_name = '320_' + f 
         cv2.imwrite(gt_dir_320 + '/' + new_name, img320)

   def rename_images(orig_jpg_dir):
      #this function renames all the original input images to
      #a sequence:  0.jpg, 1.jpg, ...
      print "Renaming input images"

      self.jpg_files = os.listdir(orig_jpg_dir)
      self.jpg_files.sort()
      
      #if the input directory does not exist, then create it
      if not os.path.exists(self.input_dir):
         os.makedirs(self.input_dir)

      #if the input annotation directory does not exist, then create it
      if not os.path.exists(sys.argv[1] + '/input_annotation'):
         os.makedirs(sys.argv[1] + '/input_annotation')

      #print jpg_files

      #for each file, copy and rename it to the input directory
      counter=0
      for f in self.jpg_files:
         new_name = format(counter, '04d') + '.jpg'
         img = cv2.imread(path.join(orig_jpg_dir,f))
         
         if counter == 139:
               print f

         cv2.imwrite(path.join(self.input_dir, new_name), img)
         input_files.append(new_name)

         x_filename = f.replace('.jpg', '.xml')
         os.system("cp " + path.join(xml_dir, x_filename) + " " + path.join(sys.argv[1], "input_annotation", format(counter, '04d') + ".xml"))

         counter += 1

   def build_annotation_images(output_dir):
      global input_files, remove_image_with_no_polygon

      temp_input_files = input_files[:]

      #check if no annotation
      counter = 0
      if remove_image_with_no_polygon == 1:
         for f in temp_input_files:
            x_filename = f.replace('.jpg', '.xml')
            e = xml.etree.ElementTree.parse(path.join(sys.argv[1], "input_annotation", x_filename).getroot())
            #print x_filename, e

            pt_list = e.iter('polygon')
            pt_list = len(list(pt_list))
            if pt_list == 0:
               #there is no polygon
               input_files.remove(f)
               print f
               counter += 1
               os.system('cd ' + sys.argv[1] + '/input; rm ' + f)
               os.system('cd ' + sys.argv[1] + '/input_annotation; rm ' + x_filename)
         print "Counter: " + str(counter)

      for f in input_files:
         polygon_list = []

         img = cv2.imread(path.join(sys.argv[1], "input", f))
         height, width, channels = img.shape
         print width,height

         #get all the points in a polygon annotation file
         x_filename = f.replace('.jpg', '.xml')
         e = xml.etree.ElementTree.parse(path.join(sys.argv[1], "input_annotation", x_filename).getroot())
         print x_filename, e

         #build a polygon
         for child in e.iter('pt'):
            x = int(child[0].text)
            y = int(child[1].text)
            if (height-y) <= 7 and (height-y) >= 3:
               print "test: " + x_filename
               #sys.exit()
            if (height-y) <= 3: #if the ground truth does not reach bottom of image
               y = height
            polygon_list.append([x,y])

         #print polygon_list

         #create new annotation image
         img_new = np.zeros((height,width,3), np.uint8)
         pts = np.array(polygon_list, np.int32)
         pts = pts.reshape((-1,1,2))

         #move in any points that are outside the image
         for i in range(len(pts)):
            #print pts[i][0]
            if (pts[i][0])[0] >= width:
               (pts[i][0])[0] = width-1
            assert (pts[i][0])[0] < width
            
            if (pts[i][0])[1] >= height:
               (pts[i][0])[1] = height-1
            assert (pts[i][0])[1] < height

         #cv2.polylines(img_new,[pts], True, (0,255,255))
         cv2.polylines(img_new,[pts], True, (255,255,255))

         img_new = fill_polygon(img_new) #fill in the polygon

         #cv2.imshow("cropped", img_new)
         #cv2.waitKey(0)
         cv2.imwrite(path.join(output_dir, 'gt_' + f), img_new)

random.seed(101)
xml_dir = sys.argv[1]+"/Annotations/users/jseng/building14"
ground_output_dir = sys.argv[1]+"/ground_output"

def run_full():
   global xml_dir, ground_output_dir

   jpg_dir = sys.argv[1]+"/Images/users/jseng/building14"
   #print files

   xml_files = os.listdir(xml_dir)
   xml_files.sort()
   #print xml_files
   output_dir = sys.argv[1]+"/augmented_output"
   if not os.path.exists(output_dir):
      os.makedirs(output_dir)

   if not os.path.exists(ground_output_dir):
      os.makedirs(ground_output_dir)
      
   rename_images(jpg_dir)
   build_annotation_images(ground_output_dir)

   #sys.exit()

   crop_images(input_dir, input_files, output_dir)
   mirror_images(input_dir, input_files, output_dir)

   build_320_240_images(sys.argv[1] + "/augmented_output")
   build_320_240_images(sys.argv[1] + "/input")
   build_320_240_gt_images(sys.argv[1] + "/ground_output")
   get_range_data([sys.argv[1] + '/320_ground_truth'], sys.argv[1] + '/320_data') #output range data for the 320x240 files

#run_full()
a = ImageAugmentor()
