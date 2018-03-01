#!/usr/bin/python

import os
import sys
import random
import datetime

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

rand_num_list = []

while len(rand_num_list) < 300:
    x = random.randint(0,num_jpg_files-1)

    if x not in rand_num_list:
        rand_num_list.append(x)

print rand_num_list
now = datetime.datetime.now()
#randomly select files
for x in range(len(rand_num_list)):
    rand_name = jpg_files[rand_num_list[x]]
    print rand_name

    #copy file to this directory
    os.system("cp " + sys.argv[1] + "/" + rand_name + " ./testdir/" + str(now.month) + '_' + str(now.day) + '_' + str(x) + ".jpg")


    
    
