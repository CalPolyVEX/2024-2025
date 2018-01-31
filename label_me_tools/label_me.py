#!/usr/bin/python

import os
import sys
import random

random.seed(101)

if len(sys.argv) <= 1:
    print "need command line arguments"

#get the files in the directory 
files = os.listdir(sys.argv[1])
#print files

#build a list of only the .jpg files
jpg_files = [f for f in files if 'jpg' in f]
print jpg_files
num_jpg_files = len(jpg_files)

#randomly select files
for x in range(100):
    rand_name = jpg_files[random.randint(0,num_jpg_files-1)]
    print rand_name

    #copy file to this directory
    os.system("cp " + sys.argv[1] + "/" + rand_name + " ./testdir/" + str(x) + ".jpg")


    
    
