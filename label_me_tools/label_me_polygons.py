#!/usr/bin/python

#usage ./label_me_polygons.py directory_of_files
#directory_of_files/Images
#directory_of_files/Annotations

import os
import sys
import random
import cv2
import xml.etree.ElementTree

random.seed(101)

if len(sys.argv) <= 1:
    print "need command line arguments"

files = os.listdir(sys.argv[1]+"/Images/users/jseng/building14")
#print files

xml_dir = sys.argv[1]+"/Annotations/users/jseng/building14"
xml_files = os.listdir(xml_dir)
#print xml_files

for x in xml_files:
    e = xml.etree.ElementTree.parse(xml_dir + "/" + x).getroot()
    print e
    for child in e.iter('pt'):
        print child[0].text
        print child[1].text


jpg_files = [f for f in files if 'jpg' in f]
print jpg_files
num_jpg_files = len(jpg_files)

sys.exit()

for x in range(100):
    rand_name = jpg_files[random.randint(0,num_jpg_files-1)]
    print rand_name

    #copy file to this directory
    os.system("cp " + sys.argv[1] + "/" + rand_name + " ./testdir/" + str(x) + ".jpg")


    
    
