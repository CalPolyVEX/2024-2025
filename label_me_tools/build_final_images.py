#!usr/bin/python

#usage ./label_me_polygons.py directory_of_files
#directory_of_files/Images
#directory_of_files/Annotations

import os, sys, random
import cv2, numpy as np

if len(sys.argv) <= 1:
    print "need command line arguments"
    sys.exit()

jpg_files = [] #a list of the original .jpg files
input_files = [] #list of the files renamed: 0.jpg, 1.jpg, ...
input_dir = sys.argv[1] + "/input"

def build_output_images(in_dir, out_dir):
    jpg_list = []
    datafile_list = []
    new_jpg_list = []

    files = os.listdir(in_dir)
    files.sort()

    for f in files:
        if '.jpg' in f:
            jpg_list.append(f)
            new_jpg_list.append('new_' + f)
        if '.txt' in f:
            datafile_list.append(f)

    #create directory for 320x240 input files
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    for i in range(len(jpg_list)):  #process each file
        img = cv2.imread(in_dir + '/' + jpg_list[i])
        print 'opening ' + jpg_list[i]

        #read in the datafile
        with open(in_dir+'/'+datafile_list[i]) as f:
            lines = f.readlines()

        f.close()
        f = [x.strip() for x in lines] #remove newlines
        f = [x.split(',') for x in f]

        new_f = []
        for x in f:
            x = [int(y) for y in x]
            if x[1] < 0:
                new_f.append((x[0],0))
            elif x[1] > 239:
                new_f.append((x[0],239))
            else:
                new_f.append((x[0],x[1]))

        for x in new_f:
            y = (x[0], x[1])
            print y
            cv2.circle(img,y,4,(0,0,255),-1)

        #write the image
        cv2.imwrite(out_dir + '/' + new_jpg_list[i], img)


output_dir = sys.argv[1] + "/320_final_inference_output"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

input_dir = sys.argv[1] + "/320_inference_input"
build_output_images(input_dir, output_dir)