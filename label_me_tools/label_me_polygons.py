#!/usr/bin/python

#usage ./label_me_polygons.py directory_of_files
#directory_of_files/Images
#directory_of_files/Annotations

import os, sys, random
import cv2, numpy as np
import xml.etree.ElementTree

def crop_images(jpg_dir, jpg_files, output_dir):
    crop_amount = 50
    file1 = jpg_files[0]
    for f in jpg_files:
        print "cropping " + f
        img = cv2.imread(jpg_dir + '/' + f)
        height, width, channels = img.shape

        crop_left_img = img[0:height, crop_amount:width] #trim left edge
        crop_right_img = img[0:height, 0:width-crop_amount] #trim right edge
        crop_top_img = img[crop_amount:height, 0:width] #trim top edge
        crop_bottom_img = img[0:height-crop_amount, 0:width] #trim bottom edge

        crop_left_img = cv2.resize(crop_left_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_right_img = cv2.resize(crop_right_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_top_img = cv2.resize(crop_top_img, (width,height), interpolation=cv2.INTER_CUBIC)
        crop_bottom_img = cv2.resize(crop_bottom_img, (width,height), interpolation=cv2.INTER_CUBIC)
        #cv2.imshow("cropped", crop_img)
        #cv2.waitKey(0)

        cv2.imwrite(output_dir + '/crop_left_test' + f, crop_left_img)
        cv2.imwrite(output_dir + '/crop_right_test' + f, crop_right_img)
        cv2.imwrite(output_dir + '/crop_top_test' + f, crop_top_img)
        cv2.imwrite(output_dir + '/crop_bottom_test' + f, crop_bottom_img)

random.seed(101)

if len(sys.argv) <= 1:
    print "need command line arguments"

jpg_dir = sys.argv[1]+"/Images/users/jseng/building14"
jpg_files = os.listdir(jpg_dir)
#print files

xml_dir = sys.argv[1]+"/Annotations/users/jseng/building14"
xml_files = os.listdir(xml_dir)
#print xml_files
output_dir = sys.argv[1]+"/output"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

crop_images(jpg_dir, jpg_files, output_dir)

for f in jpg_files:
    polygon_list = []

    #get all the points in a polygon annotation file
    x_filename = f.replace('.jpg', '.xml')
    e = xml.etree.ElementTree.parse(xml_dir + "/" + x_filename).getroot()
    print e
    for child in e.iter('pt'):
        polygon_list.append([int(child[0].text), int(child[1].text)])

    print polygon_list

    img = cv2.imread(jpg_dir + '/' + f)
    height, width, channels = img.shape
    print width,height

    #create new annotation image
    img_new = np.zeros((height,width,3), np.uint8)
    pts = np.array(polygon_list, np.int32)
    pts = pts.reshape((-1,1,2))
    #cv2.polylines(img_new,[pts], True, (0,255,255))
    cv2.fillConvexPoly(img_new,pts, (0,255,255))

    cv2.imwrite(output_dir + '/test' + f, img_new)

#jpg_files = [f for f in files if 'jpg' in f]
#print jpg_files
#num_jpg_files = len(jpg_files)

sys.exit()

#os.system("cp " + sys.argv[1] + "/" + rand_name + " ./testdir/" + str(x) + ".jpg")
