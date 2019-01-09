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
from scipy import misc

width = 480

class ImageAugmentor:
   def __init__(self, collection_dir, width):
      os.system('rm -f -r ' + path.join(collection_dir,'input','output'))
      self.file_mapping = {}
      self.width = width
      self.height = 270
      self.image_counter = 0
      self.num_valid_images = 0 #number of valid images
      self.collection_dir = collection_dir
      self.jpg_files = [] #a list of the original .jpg files
      self.input_files = [] #list of the files renamed: 0.jpg, 1.jpg, ...
      self.input_dir = path.join(sys.argv[1], "input")
      self.remove_image_with_no_polygon = 1
      self.xml_dir = path.join(collection_dir,"Annotations/users/jseng/building14_hdr")
      self.orig_jpg_dir = path.join(collection_dir,"Images/users/jseng/building14_hdr")
      self.ground_output_dir = path.join(collection_dir,"ground_output")
      self.test_image_dir = path.join(collection_dir,str(self.width) + "_test_input")
      self.test_data_dir = path.join(collection_dir,str(self.width) + "_test_data")

   def noise_generator (self, noise_type,image):
      """
      Generate noise to a given Image based on required noise type

      Input parameters:
         image: ndarray (input image data. It will be converted to float)

         noise_type: string
               'gauss'        Gaussian-distrituion based noise
               'poission'     Poission-distribution based noise
               's&p'          Salt and Pepper noise, 0 or 1
               'speckle'      Multiplicative noise using out = image + n*image
                              where n is uniform noise with specified mean & variance
      """
      row,col,ch= image.shape
      if noise_type == "gauss":
         mean = 0.0
         var = .1
         sigma = var**0.5
         gauss = np.array(image.shape)
         gauss = np.random.normal(mean,sigma,(row,col,ch))
         gauss = gauss.reshape(row,col,ch)
         noisy = image + gauss
         return noisy.astype('uint8')
      elif noise_type == "s&p":
         s_vs_p = 0.5
         amount = 0.004
         out = image
         # Generate Salt '1' noise
         num_salt = np.ceil(amount * image.size * s_vs_p)
         coords = [np.random.randint(0, i - 1, int(num_salt))
               for i in image.shape]
         out[coords] = 255
         # Generate Pepper '0' noise
         num_pepper = np.ceil(amount* image.size * (1. - s_vs_p))
         coords = [np.random.randint(0, i - 1, int(num_pepper))
               for i in image.shape]
         out[coords] = 0
         return out
      elif noise_type == "poisson":
         vals = len(np.unique(image))
         vals = 2 ** np.ceil(np.log2(vals))
         noisy = np.random.poisson(image * vals) / float(vals)
         return noisy
      elif noise_type =="speckle":
         gauss = .1 * np.random.randn(row,col,ch)
         gauss = gauss.reshape(row,col,ch)
         noisy = image + image * gauss
         return noisy
      else:
         return image
   
   def augment_test(self,num):
      p = Augmentor.Pipeline(self.input_dir)
      # Point to a directory containing ground truth data.
      # Images with the same file names will be added as ground truth data
      # and augmented in parallel to the original data.
      p.ground_truth(self.ground_output_dir)
      # Add operations to the pipeline as normal:
      #p.rotate(probability=1, max_left_rotation=5, max_right_rotation=5)
      #p.flip_left_right(probability=0.5)
      p.zoom_random(probability=0.5, percentage_area=0.9)
      #p.zoom_random(probability=0.5, percentage_area=0.8)
      p.skew_left_right(probability=0.5, magnitude=.5)
      p.crop_random(probability=.75, percentage_area=.9)
      p.resize(probability=1.0, width=self.width, height=self.height)
      p.sample(num, multi_threaded=False)
      
      files = os.listdir(path.join(self.input_dir,'output'))
      files.sort()
      original_files = [x for x in files if 'original' in x]
      gt_files = [x for x in files if 'ground' in x]

      for x in original_files:
         new_name = x.replace('input_original_','')
         new_name = str(self.width) + '_' + format(self.image_counter, '05d') + '.jpg'
         os.system('cd ' + self.input_dir + '/output; mv ' + x + ' ../../' + str(self.width) + '_input/' + new_name)
         self.image_counter += 1
      self.image_counter -= len(original_files)

      for x in gt_files:
         new_name = x.replace('_groundtruth_(1)_input_','')
         new_name = str(self.width) + '_' + format(self.image_counter, '05d') + '.jpg'
         x= x.replace('(','\(')
         x= x.replace(')','\)')
         print new_name
         #sys.exit(0)
         os.system('cd ' + self.input_dir + '/output; mv ' + x + ' ../../' + str(self.width) + '_ground_truth/' + new_name)
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

         #testing JS
         if 1 == 0:
            img_gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            #cv2.imshow("Output", img_gray)
            #cv2.waitKey(0)
            (_, cnts, _) = cv2.findContours(img_gray, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_TC89_L1)
            #print cnts
            #sys.exit()
            for c in cnts:
               epsilon = .005*cv2.arcLength(c,True)
               approx = cv2.approxPolyDP(c,epsilon, True)
               #print approx.shape
               cv2.drawContours(img, [approx], -1, (0,255,0), 4)
            #cv2.imshow("Output", img)
            #cv2.waitKey(0)

         data=[]
         f_out = open(out + '/' + f.replace('.jpg', '.txt'), 'w')

         #run from 0 to the right edge of the image
         for x in range(5,width,step):
            temp = height-2  #start at the bottom of the image
            while temp >= 0:
               pixel = img[temp,x]
               if pixel[0] < 150 or temp == 0: #if the pixel is black or reach the top of image
                     #sys.stdout.write('%d, ' % temp)
                     #cv2.circle(blank_image,(x,temp),5,(0,0,255),-1)
                     data.append((x,temp))
                     f_out.write(str(x) + ',' + str(temp) + '\n')
                     #cv2.circle(img, (x,temp), 5, (0,0,255), -1)
                     #blank_image[temp,x] = [0,0,255]
                     found=1
                     if validate==1: #to validate range data, draw circles on original image
                        #print 'validate range data'
                        img_path = path.join(sys.argv[1], str(self.width) + '_input')
                        img_path = path.join(img_path, f)
                        #print img_path
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
      self.num_valid_images = len(files)

      #create directory for 480x270 input files
      input_dir_480 = path.join(self.collection_dir, str(self.width) + "_input")
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
      gt_dir_480 = sys.argv[1]+"/" + str(self.width) + "_ground_truth"
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
      input_dir_480 = path.join(self.collection_dir, str(self.width) + "_input")
      files = os.listdir(input_dir_480)
      for x in files:
         prob = random.randint(0,9)
         fullname = path.join(input_dir_480,x)
         #print fullname
         if prob <= 2:
            os.system('convert -brightness-contrast 10x10 ' + fullname + ' ' + fullname)
         elif prob <= 4:
            os.system('convert -brightness-contrast -10x-10 ' + fullname + ' ' + fullname)

         if 1 == 1: #add noise to images
            prob = random.randint(0,9)
            if prob <= 3:
               img = misc.imread(fullname)
               img = self.noise_generator('s&p',img)
               misc.imsave(fullname,img)
            elif prob <= 6:
               img = misc.imread(fullname)
               img = self.noise_generator('speckle',img)
               misc.imsave(fullname,img)

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
            try:
               e = xml.etree.ElementTree.parse(path.join(self.collection_dir, "input_annotation", x_filename)).getroot()
               #print x_filename, e
            except:
               print "XML tree exception: " + x_filename 
               pt_list = 0
            else:
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

            #print "polygon number of points: " + str(len(pts))
            cv2.polylines(img_new,[pts], True, 255)

         img_new = self.fill_polygon(img_new) #fill in the polygon

         #cv2.imshow("cropped", img_new)
         #cv2.waitKey(0)
         cv2.imwrite(path.join(self.ground_output_dir, f), img_new)

   #############################################################
   def build_test_set(self):
      print '---build_test_set---'
      #if the input directory does not exist, then create it
      if not os.path.exists(self.test_image_dir):
         os.makedirs(self.test_image_dir)
         os.makedirs(self.test_data_dir)

      input_dir_480 = path.join(self.collection_dir, str(self.width) + "_input")
      data_dir_480 = path.join(self.collection_dir, str(self.width) + "_data")
      files = os.listdir(input_dir_480)
      data = os.listdir(data_dir_480)
      files.sort()
      data.sort()
      files = files[0:self.num_valid_images]
      data = data[0:self.num_valid_images]

      for f in files:
         os.system('cd ' + sys.argv[1] + '/480_input; cp ' + f + ' ../480_test_input')
      for d in data:
         os.system('cd ' + sys.argv[1] + '/480_data; cp ' + d + ' ../480_test_data')



   def upload(self):
      img_cmd = 'rm -f img.tar.gz; tar zcf img.tar.gz -C ' + self.collection_dir + '/' + str(self.width) + '_input/ .'
      test_img_cmd = 'rm -f test_img.tar.gz; tar zcf test_img.tar.gz -C ' + self.collection_dir + '/' + str(self.width) + '_test_input/ .'

      data_cmd = 'rm -f data.tar.gz; tar zcf data.tar.gz -C ' + self.collection_dir + '/' + str(self.width) + '_data/ .'
      test_data_cmd = 'rm -f test_data.tar.gz; tar zcf test_data.tar.gz -C ' + self.collection_dir + '/' + str(self.width) + '_test_data/ .'

      img_cp = 'scp img.tar.gz unix3.csc.calpoly.edu:/home/jseng/ue4/ground_detection/gpu_code/' + str(self.width) + '_images'
      test_img_cp = 'scp test_img.tar.gz unix3.csc.calpoly.edu:/home/jseng/ue4/ground_detection/gpu_code/' + str(self.width) + '_test_images'
      data_cp = 'scp data.tar.gz unix3.csc.calpoly.edu:/home/jseng/ue4/ground_detection/gpu_code/' + str(self.width) + '_data'
      test_data_cp = 'scp test_data.tar.gz unix3.csc.calpoly.edu:/home/jseng/ue4/ground_detection/gpu_code/' + str(self.width) + '_test_data'
      
      os.system(img_cmd)
      os.system(data_cmd)
      os.system(test_img_cmd)
      os.system(test_data_cmd)

      os.system(img_cp)
      os.system(data_cp)
      os.system(test_img_cp)
      os.system(test_data_cp)
      os.system('rm -f img.tar.gz')
      os.system('rm -f data.tar.gz')
      os.system('rm -f test_img.tar.gz')
      os.system('rm -f test_data.tar.gz')

      print '---extracting remote files---'
      img_unzip = 'ssh unix3.csc.calpoly.edu \'cd ue4/ground_detection/gpu_code/' + str(self.width) + '_images; rm -f *.jpg; tar -xzf img.tar.gz; rm img.tar.gz\''
      test_img_unzip = 'ssh unix3.csc.calpoly.edu \'cd ue4/ground_detection/gpu_code/' + str(self.width) + '_test_images; rm -f *.jpg; tar -xzf test_img.tar.gz; rm test_img.tar.gz\''
      data_unzip = 'ssh unix3.csc.calpoly.edu \'cd ue4/ground_detection/gpu_code/' + str(self.width) + '_data; rm -f *.txt; tar -xzf data.tar.gz; rm data.tar.gz\''
      test_data_unzip = 'ssh unix3.csc.calpoly.edu \'cd ue4/ground_detection/gpu_code/' + str(self.width) + '_test_data; rm -f *.txt; tar -xzf test_data.tar.gz; rm test_data.tar.gz\''
      os.system(img_unzip)
      os.system(data_unzip)

random.seed(101)

if __name__ == '__main__':
   if len(sys.argv) <= 1:
      print "need command line arguments"
      #sys.exit()
   elif sys.argv[1] == 'clean':
      print "clean"
      os.system('cd ' + sys.argv[2] + '/' + str(width) + '_test_input; rm -f *.jpg; rm -f *.zip')
      os.system('cd ' + sys.argv[2] + '/' + str(width) + '_test_data; rm -f *.jpg; rm -f *.zip')
      os.system('cd ' + sys.argv[2] + '/' + str(width) + '_input; rm -f *.jpg; rm -f *.zip')
      os.system('cd ' + sys.argv[2] + '/' + str(width) + '_data; rm -f *.txt; rm -f *.zip')
      os.system('cd ' + sys.argv[2] + '/' + str(width) + '_ground_truth; rm -f *.jpg')
      os.system('cd ' + sys.argv[2] + '/input; rm -f *.jpg; rm -f -r output')
      os.system('cd ' + sys.argv[2] + '/input_annotation; rm -f *.xml')
      os.system('cd ' + sys.argv[2] + '/ground_output; rm -f *.jpg')
      sys.exit()
   elif sys.argv[1] == 'upload':
      a = ImageAugmentor(sys.argv[2], width)
      a.upload()
      sys.exit()

   a = ImageAugmentor(sys.argv[1], width)
   a.rename_images()
   a.build_annotation_images()
   a.build_480_270_images()
   a.build_480_270_gt_images()
   a.augment_test(52)
   #a.augment_test(300)
   a.get_range_data(path.join(sys.argv[1], str(width) + '_ground_truth'), path.join(sys.argv[1], str(width) + '_data'), 0)
   a.build_test_set()
   a.adjust_color()
