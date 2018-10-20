#!usr/bin/python

#usage ./label_me_polygons.py directory_of_files
#directory_of_files/Images
#directory_of_files/Annotations

import os, sys, random
from os import path
import cv2, numpy as np
import xml.etree.ElementTree
import Augmentor, random, time
from datetime import datetime

class ImageAugmentor:
   def __init__(self, collection_dir):
      os.system('rm -f -r ' + path.join(collection_dir,'input','output'))
      self.file_mapping = {}
      self.width = 480
      self.height = 270
      self.image_counter = 0
      self.collection_dir = collection_dir
      self.jpg_files = [] #a list of the original .jpg files
      self.input_files = [] #list of the files renamed: 0.jpg, 1.jpg, ...
      self.input_dir = path.join(sys.argv[1], "input")
      self.remove_image_with_no_polygon = 1
      self.xml_dir = path.join(collection_dir,"Annotations/users/jseng/building14_hdr")
      self.orig_jpg_dir = path.join(collection_dir,"Images/users/jseng/building14_hdr")
      self.ground_output_dir = path.join(collection_dir,"ground_output")
   
   def augment_test(self,num):
      p = Augmentor.Pipeline(self.input_dir)
      # Point to a directory containing ground truth data.
      # Images with the same file names will be added as ground truth data
      # and augmented in parallel to the original data.
      p.ground_truth(self.ground_output_dir)
      # Add operations to the pipeline as normal:
      #p.rotate(probability=1, max_left_rotation=5, max_right_rotation=5)
      #p.flip_left_right(probability=0.5)
      p.zoom_random(probability=0.5, percentage_area=0.95)
      p.zoom_random(probability=0.5, percentage_area=0.8)
      p.skew_left_right(probability=0.5, magnitude=.5)
      p.crop_random(probability=.75, percentage_area=.9)
      p.resize(probability=1.0, width=self.width, height=self.height)
      p.sample(num)
      
      files = os.listdir(path.join(self.input_dir,'output'))
      files.sort()
      original_files = [x for x in files if 'original' in x]
      gt_files = [x for x in files if 'ground' in x]

      for x in original_files:
         new_name = x.replace('input_original_','')
         new_name = '480_' + format(self.image_counter, '05d') + '.jpg'
         os.system('cd ' + self.input_dir + '/output; mv ' + x + ' ../../480_input/' + new_name)
         self.image_counter += 1
      self.image_counter -= len(original_files)

      for x in gt_files:
         new_name = x.replace('_groundtruth_(1)_input_','')
         new_name = '480_' + format(self.image_counter, '05d') + '.jpg'
         x= x.replace('(','\(')
         x= x.replace(')','\)')
         print new_name
         #sys.exit(0)
         os.system('cd ' + self.input_dir + '/output; mv ' + x + ' ../../480_ground_truth/' + new_name)
         self.image_counter += 1

   #############################################################
   #fill_polygon function
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

   #############################################################
   #create the actual data to feed to neural network
   def get_range_data(self, dirs, out, validate):
      print "---Generating data files for images---"

      if not os.path.exists(out):
         os.makedirs(out)

      files = os.listdir(dirs)
      files.sort()
      for f in files:
         #print f
         img = cv2.imread(path.join(dirs,f))
         height, width, channels = img.shape
         step = width / 47
         #blank_image = np.zeros((height,width,3), np.uint8)

         data=[]
         f_out = open(out + '/' + f.replace('.jpg', '.txt'), 'w')

         #run from 0 to the right edge of the image
         for x in range(5,width,step):
            temp = height-2  #start at the bottom of the image
            while temp >= 0:
               pixel = img[temp,x]
               if pixel[0] == 0 or temp == 0: #if the pixel is black or reach the top of image
                     #sys.stdout.write('%d, ' % temp)
                     #cv2.circle(blank_image,(x,temp),5,(0,0,255),-1)
                     data.append((x,temp))
                     f_out.write(str(x) + ',' + str(temp) + '\n')
                     #cv2.circle(img, (x,temp), 5, (0,0,255), -1)
                     #blank_image[temp,x] = [0,0,255]
                     found=1
                     if validate==1: #to validate range data, draw circles on original image
                        print 'validate range data'
                        img_path = path.join(sys.argv[1],'480_input')
                        img_path = path.join(img_path, f)
                        print img_path
                        img2 = cv2.imread(img_path)
                        cv2.circle(img2,(x,temp),4,(0,0,255),-1)
                        cv2.imwrite(img_path,img2)
                     break
               temp = temp - 1

         f_out.close()
         #print data
         #cv2.imwrite(path.join(dirs,f), img)
      
   #############################################################
   def build_480_270_images(self):
      print "Converting input images to 480x270"
      files = os.listdir(self.input_dir)
      files.sort()

      #create directory for 480x270 input files
      input_dir_480 = path.join(self.collection_dir,"480_input")
      if not os.path.exists(input_dir_480):
         os.makedirs(input_dir_480)

      for f in files:  #resize all the input files
         img = cv2.imread(path.join(self.input_dir, f))
         img480 = cv2.resize(img,(self.width,self.height), interpolation=cv2.INTER_CUBIC)
         new_name = str(self.width) + '_' + f
         cv2.imwrite(path.join(input_dir_480, new_name), img480)

   #############################################################
   def build_480_270_gt_images(self):
      print "Converting ground truth images to 480x270"
      gt_files = os.listdir(self.ground_output_dir)
      gt_files.sort()

      #create directory for 480x270 ground truth files
      gt_dir_480 = sys.argv[1]+"/480_ground_truth"
      if not os.path.exists(gt_dir_480):
         os.makedirs(gt_dir_480)

      for f in gt_files:  #resize all the ground_truth files
         img = cv2.imread(path.join(self.ground_output_dir,f))
         img480 = cv2.resize(img,(self.width,self.height), interpolation=cv2.INTER_CUBIC)
         new_name = str(self.width) + '_' + f 
         cv2.imwrite(gt_dir_480 + '/' + new_name, img480)

   #############################################################
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
         new_name = format(self.image_counter, '05d') + '.jpg'
         img = cv2.imread(path.join(self.orig_jpg_dir,f))
         
#         error_list = [110,456,466,467,505,507,508,511,512]
#         if self.image_counter in error_list:
#               print "ground_truth_error: " + str(f) + " -- " + str(self.image_counter)
#               #sys.exit()

         cv2.imwrite(path.join(self.input_dir, new_name), img)
         self.input_files.append(new_name)
         self.file_mapping[new_name] = f

         #renaming xml files
         x_filename = f.replace('.jpg', '.xml')
         os.system("cp " + path.join(self.xml_dir, x_filename) + " " + path.join(self.collection_dir, "input_annotation", format(self.image_counter, '05d') + ".xml"))

         self.image_counter += 1

   #############################################################
   def adjust_color(self):
      random.seed(datetime.now())
      input_dir_480 = path.join(self.collection_dir,"480_input")
      files = os.listdir(input_dir_480)
      for x in files:
         prob = random.randint(0,9)
         fullname = path.join(input_dir_480,x)
         print fullname
         if prob <= 2:
            os.system('convert -brightness-contrast 15x15 ' + fullname + ' ' + fullname)
         elif prob <= 4:
            os.system('convert -brightness-contrast -15x-15 ' + fullname + ' ' + fullname)

   #############################################################
   def build_annotation_images(self):
      print '---build_annotation_images---'
      #if the input directory does not exist, then create it
      if not os.path.exists(self.ground_output_dir):
         os.makedirs(self.ground_output_dir)

      temp_input_files = self.input_files[:]

      #check if no annotation
      counter = 0
      if self.remove_image_with_no_polygon == 1:
         for f in temp_input_files:
            x_filename = f.replace('.jpg', '.xml')
            #print x_filename
            e = xml.etree.ElementTree.parse(path.join(self.collection_dir, "input_annotation", x_filename)).getroot()
            #print x_filename, e

            pt_list = e.iter('polygon')
            pt_list = len(list(pt_list))
            if pt_list == 0:
               #there is no polygon
               self.input_files.remove(f)
               print 'no polygon: ' + f
               counter += 1
               os.system('cd ' + sys.argv[1] + '/input; rm ' + f)
               os.system('cd ' + sys.argv[1] + '/input_annotation; rm ' + x_filename)
         print "number of files with no polygon: " + str(counter)

      for f in self.input_files:
         polygon_list = []

         img = cv2.imread(path.join(sys.argv[1], "input", f))
         height, width, channels = img.shape
         #print width,height

         #get all the points in a polygon annotation file
         x_filename = f.replace('.jpg', '.xml')
         e = xml.etree.ElementTree.parse(path.join(self.collection_dir, "input_annotation", x_filename)).getroot()
         #print x_filename, e

         for child in e: #for each child in the root
            if child.tag == "object":
               deleted = int(child.find("deleted").text) #find if the polygon was deleted
               if deleted != 1:
                  point_list=[]
                  #build a polygon
                  for pt_child in child.iter('pt'):
                     x = int(pt_child[0].text)
                     y = int(pt_child[1].text)
                     if ((height-y) <= 7 and (height-y) >= 3) and (x < 5 or x > (width-5)):
                        print "error: " + x_filename + ',' + self.file_mapping[f]
                        #sys.exit()
                     if (height-y) <= 3: #if the ground truth does not reach bottom of image
                        y = height
                     point_list.append([x,y])
                  polygon_list.append(point_list)
               else:
                  print "found deleted polygon in file: " + x_filename
                           
         #create new annotation image
         img_new = np.zeros((height,width,1), np.uint8)

         for polygon in polygon_list:
            pts = np.array(polygon, np.int32)
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

            cv2.polylines(img_new,[pts], True, 255)

         img_new = self.fill_polygon(img_new) #fill in the polygon

         #cv2.imshow("cropped", img_new)
         #cv2.waitKey(0)
         cv2.imwrite(path.join(self.ground_output_dir, f), img_new)

   def upload(self):
      img_cmd = 'rm -f img.zip; zip -j -q img.zip ' + self.collection_dir + '/480_input/*.jpg'
      img_cmd = 'rm -f img.tar.gz; tar zcvf img.tar.gz -C ' + self.collection_dir + '/480_input/ .'

      data_cmd = 'rm -f data.zip; zip -j -q data.zip ' + self.collection_dir + '/480_data/*.txt'
      data_cmd = 'rm -f data.tar.gz; tar zcvf data.tar.gz -C ' + self.collection_dir + '/480_data/ .'

      img_cp = 'scp img.tar.gz unix3.csc.calpoly.edu:/home/jseng/ue4/ground_detection/gpu_code/480_images'
      data_cp = 'scp data.tar.gz unix3.csc.calpoly.edu:/home/jseng/ue4/ground_detection/gpu_code/480_data'
      
      os.system(img_cmd)
      os.system(data_cmd)
      os.system(img_cp)
      os.system(data_cp)
      os.system('rm -f img.tar.gz')
      os.system('rm -f data.tar.gz')

      print '---extracting remote files---'
      img_unzip = 'ssh unix3.csc.calpoly.edu \'cd ue4/ground_detection/gpu_code/480_images; rm -f *.jpg; tar -xzf img.tar.gz; rm img.tar.gz\''
      data_unzip = 'ssh unix3.csc.calpoly.edu \'cd ue4/ground_detection/gpu_code/480_data; rm -f *.txt; tar -xzf data.tar.gz; rm data.tar.gz\''
      os.system(img_unzip)
      os.system(data_unzip)

random.seed(101)

if __name__ == '__main__':
   if len(sys.argv) <= 1:
      print "need command line arguments"
      #sys.exit()
   elif sys.argv[1] == 'clean':
      print "clean"
      os.system('cd ' + sys.argv[2] + '/480_input; rm -f *.jpg; rm -f *.zip')
      os.system('cd ' + sys.argv[2] + '/480_data; rm -f *.txt; rm -f *.zip')
      os.system('cd ' + sys.argv[2] + '/480_ground_truth; rm -f *.jpg')
      os.system('cd ' + sys.argv[2] + '/480_inference_input; rm -f *.jpg')
      os.system('cd ' + sys.argv[2] + '/480_final_inference_output; rm -f *.jpg')
      os.system('cd ' + sys.argv[2] + '/input; rm -f *.jpg; rm -f -r output')
      os.system('cd ' + sys.argv[2] + '/input_annotation; rm -f *.xml')
      os.system('cd ' + sys.argv[2] + '/ground_output; rm -f *.jpg')
      os.system('cd ' + sys.argv[2] + '/augmented_output; rm -f *.jpg')
      sys.exit()

   a = ImageAugmentor(sys.argv[1])
   a.upload()
   #a.adjust_color()
   sys.exit()
   a.rename_images()
   a.build_annotation_images()
   a.build_480_270_images()
   a.build_480_270_gt_images()
   a.augment_test(50000)
   a.get_range_data(path.join(sys.argv[1],'480_ground_truth'), path.join(sys.argv[1],'480_data'), 0)
   a.adjust_color()
   #a.upload()
