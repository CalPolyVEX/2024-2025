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

#os.system('roslaunch jet_launcher zed_rtabmap.launch ')
my_env = os.environ.copy()
my_env["PATH"] = "/opt/ros/melodic/bin:/home/jseng/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

#rtabmap-reprocess "8am_output1.db;8am_output2.db;8am_output3.db;8_30am_output1.db;9am_output1.db;10am_output1.db;10am_output2.db;10am_output3.db;12pm_output1.db;2pm_output1.db" "output.db"

s = "\""
for i in range(len(l)):
   s = s + str(i+1) + '.db;'
s = s[:-1]
s = s + '\"'
print s

#process = subprocess.Popen(["roslaunch", "jet_launcher", "zed_rtabmap.launch", "sim:=true"], env=my_env, close_fds=True)
command = 'cd /home/jseng/.ros; rtabmap-reprocess ' + s + ' \"output.db\"' 
print command

os.system(command)
