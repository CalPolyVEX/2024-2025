#!/usr/bin/python

#This script automates running roslaunch and rtabmap to build the
#database files

import os
import subprocess
import time
import sys

l = [ '2019-05-25-08-23-46.bag',
      '2019-05-27-06-37-09.bag',
      '2019-05-27-06-39-16.bag',
      '2019-05-27-06-41-23.bag',
      '2019-05-27-06-43-28.bag']

db_files = ['1.db',
            '2.db',
            '3.db',
            '4.db',
            '5.db']

if len(sys.argv) > 1:
    #has command line arguments
    l = sys.argv[1:]
    print l
    db_files = []
    for i in range(len(l)):
        name = str(i+1) + '.db'
        db_files.append(name)
    print db_files

print "test"
#os.system('roslaunch jet_launcher zed_rtabmap.launch ')
my_env = os.environ.copy()
my_env["PATH"] = "/opt/ros/melodic/bin:/home/jseng/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

for i in range(len(l)):
   #process = subprocess.Popen(["roslaunch", "jet_launcher", "zed_rtabmap.launch", "sim:=true"], env=my_env, close_fds=True)
   process = subprocess.Popen(["roslaunch", "jet_launcher", "zed_rtabmap.launch"], env=my_env, close_fds=True)
   print process
   time.sleep(20)
   print process.pid
   command = 'cd /home/jseng/.ros; rosbag play --clock -d 2 ' + l[i] + ';'
   os.system(command)
   print command + ',' + db_files[i]
   time.sleep(5)
   process.terminate()
   time.sleep(10)
   copy_command = 'cd /home/jseng/.ros; cp rtabmap.db ' + db_files[i]
   os.system(copy_command)
   print copy_command

   time.sleep(10)
