#!usr/bin/python

#usage ./label_me_polygons.py directory_of_files
#directory_of_files/Images
#directory_of_files/Annotations

import os, sys, random
import cv2, numpy as np
import xml.etree.ElementTree

if len(sys.argv) <= 1:
    print "need command line arguments"
    sys.exit()

jpg_files = [] #a list of the original .jpg files
input_files = [] #list of the files renamed: 0.jpg, 1.jpg, ...
input_dir = sys.argv[1] + "/input"

def fill_polygon(img):
    height, width, channels = img.shape

    #build a mask that is 2 pixels wider and taller
    mask=np.zeros((height+2, width+2), np.uint8)
    cv2.floodFill(img, mask, (0,0), 255)

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
    for y in range(height):
        for x in range(width):
            if img[y,x][0] != 255:
                img[y,x] = [255,255,255]
            else:
                img[y,x] = [0,0,0]

    return img

def crop_images(jpg_dir, jpg_files, output_dir):
    crop_amount = 50

    for f in jpg_files:
        print "cropping " + f
        img = cv2.imread(jpg_dir + '/' + f)
        gt_img = cv2.imread(ground_output_dir + '/gt_' + f)
        height, width, channels = img.shape

        crop_left_img = img[0:height, crop_amount:width] #trim left edge
        crop_right_img = img[0:height, 0:width-crop_amount] #trim right edge
        crop_top_img = img[crop_amount:height, 0:width] #trim top edge
        crop_bottom_img = img[0:height-crop_amount, 0:width] #trim bottom edge

        crop_gt_left_img = gt_img[0:height, crop_amount:width] #trim left edge
        crop_gt_right_img = gt_img[0:height, 0:width-crop_amount] #trim right edge
        crop_gt_top_img = gt_img[crop_amount:height, 0:width] #trim top edge
        crop_gt_bottom_img = gt_img[0:height-crop_amount, 0:width] #trim bottom edge
        
        crop_left_img = cv2.resize(crop_left_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_right_img = cv2.resize(crop_right_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_top_img = cv2.resize(crop_top_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_bottom_img = cv2.resize(crop_bottom_img, (width,height), interpolation=cv2.INTER_CUBIC)

        crop_gt_left_img = cv2.resize(crop_gt_left_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_gt_right_img = cv2.resize(crop_gt_right_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_gt_top_img = cv2.resize(crop_gt_top_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_gt_bottom_img = cv2.resize(crop_gt_bottom_img, (width,height), interpolation=cv2.INTER_CUBIC)

        cv2.imwrite(output_dir + '/crop_left_' + f, crop_left_img)
        cv2.imwrite(output_dir + '/crop_right_' + f, crop_right_img)
        cv2.imwrite(output_dir + '/crop_top_' + f, crop_top_img)
        cv2.imwrite(output_dir + '/crop_bottom_' + f, crop_bottom_img)

        cv2.imwrite(ground_output_dir + '/crop_gt_left_' + f, crop_gt_left_img)
        cv2.imwrite(ground_output_dir + '/crop_gt_right_' + f, crop_gt_right_img)
        cv2.imwrite(ground_output_dir + '/crop_gt_top_' + f, crop_gt_top_img)
        cv2.imwrite(ground_output_dir + '/crop_gt_bottom_' + f, crop_gt_bottom_img)

        crop_img = cv2.add(crop_left_img, crop_gt_left_img)
        cv2.imshow("cropped", crop_img)
        #cv2.waitKey(0)

def mirror_images(jpg_dir, jpg_files, output_dir):
    for f in jpg_files:
        print "mirroring " + f
        img = cv2.imread(jpg_dir + '/' + f)
        gt_img = cv2.imread(ground_output_dir + '/gt_' + f)
        height, width, channels = img.shape

        mirror_img = cv2.flip(img,1)
        gt_mirror_img = cv2.flip(gt_img,1)
        #cv2.imshow("mirrored", mirror_img)
        #cv2.waitKey(0)

        cv2.imwrite(output_dir + '/mirror_' + f, mirror_img)
        cv2.imwrite(ground_output_dir + '/mirror_gt_' + f, gt_mirror_img)

def get_range_data(dirs):
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
            blank_image = np.zeros((height,width,3), np.uint8)

            for x in range(0,width,step):
                temp = height-1
                while temp > 0:
                    pixel = img[temp,x]
                    temp = temp - 1
                    if pixel[0] == 0:
                        #sys.stdout.write('%d, ' % temp)
                        cv2.circle(blank_image,(x,temp),5,(0,0,255),-1)
                        #blank_image[temp,x] = [0,0,255]
                        break
            #print ''
            cv2.imwrite(ground_output_dir + '/lidar_' + f, blank_image)
        
def rename_images(jpg_dir):
    #this function renames all the original input images to
    #a sequence:  0.jpg, 1.jpg, ...
    global jpg_files, input_files, xml_dir
    counter=0

    jpg_files = os.listdir(jpg_dir)
    jpg_files.sort()
    
    #if the input directory does not exist, then create it
    if not os.path.exists(input_dir):
        os.makedirs(input_dir)

    #if the input annotation directory does not exist, then create it
    if not os.path.exists(sys.argv[1] + '/input_annotation'):
        os.makedirs(sys.argv[1] + '/input_annotation')

    #print jpg_files

    #for each file, copy and rename it to the input directory
    for f in jpg_files:
        new_name = str(counter) + '.jpg'
        img = cv2.imread(jpg_dir + '/' + f)

        cv2.imwrite(sys.argv[1] + '/input/' + new_name, img)
        input_files.append(new_name)

        x_filename = f.replace('.jpg', '.xml')
        #os.system("cp " + sys.argv[1] + "/" + rand_name + " ./testdir/" + str(x) + ".jpg")
        os.system("cp " + xml_dir + '/' + x_filename + " " + sys.argv[1] + "/input_annotation/" + str(counter) + ".xml")

        counter += 1

def build_annotation_images(output_dir):
    global input_files

    for f in input_files:
        polygon_list = []

        img = cv2.imread(sys.argv[1] + "/input/" + f)
        height, width, channels = img.shape
        print width,height

        #get all the points in a polygon annotation file
        x_filename = f.replace('.jpg', '.xml')
        e = xml.etree.ElementTree.parse(sys.argv[1] + "/input_annotation/" + x_filename).getroot()
        print x_filename, e

        #build a polygon
        for child in e.iter('pt'):
            x = int(child[0].text)
            y = int(child[1].text)
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

        cv2.polylines(img_new,[pts], True, (0,255,255))

        img_new = fill_polygon(img_new) #fill in the polygon

        #cv2.imshow("cropped", img_new)
        #cv2.waitKey(0)
        cv2.imwrite(output_dir + '/gt_' + f, img_new)

random.seed(101)

jpg_dir = sys.argv[1]+"/Images/users/jseng/building14"
#print files

xml_dir = sys.argv[1]+"/Annotations/users/jseng/building14"
xml_files = os.listdir(xml_dir)
xml_files.sort()
#print xml_files
output_dir = sys.argv[1]+"/augmented_output"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

ground_output_dir = sys.argv[1]+"/ground_output"
if not os.path.exists(ground_output_dir):
    os.makedirs(ground_output_dir)
    
#rename_images(jpg_dir)
#build_annotation_images(ground_output_dir)
#crop_images(input_dir, input_files, output_dir)
#mirror_images(input_dir, input_files, output_dir)
get_range_data([ground_output_dir])
