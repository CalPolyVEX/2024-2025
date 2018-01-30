#!/usr/bin/python

#usage ./label_me_polygons.py directory_of_files
#directory_of_files/Images
#directory_of_files/Annotations

import os, sys, random
import cv2, numpy as np
import xml.etree.ElementTree

random.seed(101)

if len(sys.argv) <= 1:
    print "need command line arguments"

jpg_dir = sys.argv[1]+"/Images/users/jseng/building14"
files = os.listdir(jpg_dir)
#print files

xml_dir = sys.argv[1]+"/Annotations/users/jseng/building14"
xml_files = os.listdir(xml_dir)
#print xml_files

for f in files:
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

    cv2.imwrite(xml_dir + '/test' + f, img_new)

jpg_files = [f for f in files if 'jpg' in f]
print jpg_files
num_jpg_files = len(jpg_files)

sys.exit()

#os.system("cp " + sys.argv[1] + "/" + rand_name + " ./testdir/" + str(x) + ".jpg")
