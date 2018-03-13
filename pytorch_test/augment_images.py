#!usr/bin/python

#usage ./label_me_polygons.py directory_of_files
#directory_of_files/Images
#directory_of_files/Annotations

import os, sys, random
from os import path
import cv2, numpy as np
import xml.etree.ElementTree
import Augmentor

class ImageAugmentor:
   def __init__(self, collection_dir):
      os.system('rm -f -r ' + path.join(collection_dir,'input','output'))
      self.width = 320
      self.height = 240
      self.image_counter = 0
      self.collection_dir = collection_dir
      self.jpg_files = [] #a list of the original .jpg files
      self.input_files = [] #list of the files renamed: 0.jpg, 1.jpg, ...
      self.input_dir = path.join(sys.argv[1], "input")
      self.remove_image_with_no_polygon = 1
      self.xml_dir = path.join(collection_dir,"Annotations/users/jseng/building14")
      self.orig_jpg_dir = path.join(collection_dir,"Images/users/jseng/building14")
      self.ground_output_dir = path.join(collection_dir,"ground_output")
   
   def augment_test(self,num):
      p = Augmentor.Pipeline(self.input_dir)
      # Point to a directory containing ground truth data.
      # Images with the same file names will be added as ground truth data
      # and augmented in parallel to the original data.
      p.ground_truth(self.ground_output_dir)
      # Add operations to the pipeline as normal:
      p.rotate(probability=1, max_left_rotation=5, max_right_rotation=5)
      p.flip_left_right(probability=0.5)
      p.zoom_random(probability=0.5, percentage_area=0.8)
      p.resize(probability=1.0, width=self.width, height=self.height)
      p.sample(num)
      
      files = os.listdir(path.join(self.input_dir,'output'))
      files.sort()
      original_files = [x for x in files if 'original' in x]
      gt_files = [x for x in files if 'ground' in x]

      for x in original_files:
         new_name = x.replace('input_original_','')
         new_name = '320_' + format(self.image_counter, '04d') + '.jpg'
         os.system('cd ' + self.input_dir + '/output; mv ' + x + ' ../../320_input/' + new_name)
         self.image_counter += 1
      self.image_counter -= len(original_files)
      for x in gt_files:
         new_name = x.replace('_groundtruth_(1)_input_','')
         new_name = '320_' + format(self.image_counter, '04d') + '.jpg'
         x= x.replace('(','\(')
         x= x.replace(')','\)')
         print new_name
         #sys.exit(0)
         os.system('cd ' + self.input_dir + '/output; mv ' + x + ' ../../320_ground_truth/' + new_name)
         self.image_counter += 1

   def fill_polygon(self, img):
      height, width, channels = img.shape

      #build a mask that is 2 pixels wider and taller

      mask=np.zeros((height+2, width+2), np.uint8)
      #cv2.floodFill(img, mask, (0,0), (255,255,255))
      cv2.floodFill(img, mask, (0,0), 255)

      #run down the left side of the image and if there is a 
      #black pixel, then do a floodfill
      for y in range(height):
         x=0
         #if img[y,x][2] != 255:
         if img[y,x] != 255:
            cv2.floodFill(img, mask, (x,y), 255)

      for y in range(height):
         x=width-1
         #if img[y,x][2] != 255:
         if img[y,x] != 255:
            cv2.floodFill(img, mask, (x,y), 255)

      #invert the image
      img = cv2.bitwise_not(img)

      return img

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
      
   def build_320_240_images(self):
      print "Converting input images to 320x240"
      files = os.listdir(self.input_dir)
      files.sort()

      #create directory for 320x240 input files
      input_dir_320 = path.join(self.collection_dir,"320_input")
      if not os.path.exists(input_dir_320):
         os.makedirs(input_dir_320)

      for f in files:  #resize all the input files
         img = cv2.imread(path.join(self.input_dir, f))
         img320 = cv2.resize(img,(self.width,self.height), interpolation=cv2.INTER_CUBIC)
         new_name = str(self.width) + '_' + f
         cv2.imwrite(path.join(input_dir_320, new_name), img320)

   def build_320_240_gt_images(self):
      print "Converting ground truth images to 320x240"
      gt_files = os.listdir(self.ground_output_dir)
      gt_files.sort()

      #create directory for 320x240 ground truth files
      gt_dir_320 = sys.argv[1]+"/320_ground_truth"
      if not os.path.exists(gt_dir_320):
         os.makedirs(gt_dir_320)

      for f in gt_files:  #resize all the ground_truth files
         img = cv2.imread(path.join(self.ground_output_dir,f))
         img320 = cv2.resize(img,(self.width,self.height), interpolation=cv2.INTER_CUBIC)
         new_name = str(self.width) + '_' + f 
         cv2.imwrite(gt_dir_320 + '/' + new_name, img320)

   def rename_images(self):
      #this function renames all the original input images to
      #a sequence:  0.jpg, 1.jpg, ...
      print "Renaming input images"

      self.jpg_files = os.listdir(self.orig_jpg_dir)
      self.jpg_files.sort()
      
      #if the input directory does not exist, then create it
      if not os.path.exists(self.input_dir):
         os.makedirs(self.input_dir)

      #if the input annotation directory does not exist, then create it
      if not os.path.exists(path.join(self.collection_dir,'input_annotation')):
         os.makedirs(path.join(self.collection_dir, 'input_annotation'))

      #for each file, copy and rename it to the input directory
      self.image_counter=0
      for f in self.jpg_files:
         new_name = format(self.image_counter, '04d') + '.jpg'
         img = cv2.imread(path.join(self.orig_jpg_dir,f))
         
         if self.image_counter == 139:
               print f

         cv2.imwrite(path.join(self.input_dir, new_name), img)
         self.input_files.append(new_name)

         #renaming xml files
         x_filename = f.replace('.jpg', '.xml')
         os.system("cp " + path.join(self.xml_dir, x_filename) + " " + path.join(self.collection_dir, "input_annotation", format(self.image_counter, '04d') + ".xml"))

         self.image_counter += 1

   def build_annotation_images(self):
      temp_input_files = self.input_files[:]

      #check if no annotation
      counter = 0
      if self.remove_image_with_no_polygon == 1:
         for f in temp_input_files:
            x_filename = f.replace('.jpg', '.xml')
            print x_filename
            e = xml.etree.ElementTree.parse(path.join(self.collection_dir, "input_annotation", x_filename)).getroot()
            #print x_filename, e

            pt_list = e.iter('polygon')
            pt_list = len(list(pt_list))
            if pt_list == 0:
               #there is no polygon
               self.input_files.remove(f)
               print f
               counter += 1
               os.system('cd ' + sys.argv[1] + '/input; rm ' + f)
               os.system('cd ' + sys.argv[1] + '/input_annotation; rm ' + x_filename)
         print "Counter: " + str(counter)

      for f in self.input_files:
         polygon_list = []

         img = cv2.imread(path.join(sys.argv[1], "input", f))
         height, width, channels = img.shape
         #print width,height

         #get all the points in a polygon annotation file
         x_filename = f.replace('.jpg', '.xml')
         e = xml.etree.ElementTree.parse(path.join(self.collection_dir, "input_annotation", x_filename)).getroot()
         #print x_filename, e

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
         img_new = np.zeros((height,width,1), np.uint8)
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
         #cv2.polylines(img_new,[pts], True, (255,255,255))
         cv2.polylines(img_new,[pts], True, 255)

         img_new = self.fill_polygon(img_new) #fill in the polygon

         #cv2.imshow("cropped", img_new)
         #cv2.waitKey(0)
         cv2.imwrite(path.join(self.ground_output_dir, f), img_new)

random.seed(101)

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

   get_range_data([sys.argv[1] + '/320_ground_truth'], sys.argv[1] + '/320_data') #output range data for the 320x240 files

#run_full()
if __name__ == '__main__':
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

   a = ImageAugmentor(sys.argv[1])
   a.rename_images()
   a.build_annotation_images()
   a.build_320_240_images()
   a.build_320_240_gt_images()
   a.augment_test(30)
