#!/usr/bin/python

#This script automates running roslaunch and rtabmap to build the
#database files

import os
import subprocess
import time

l = [ '2019-01-26-15-56-22.bag',
         '2019-01-26-16-00-20.bag',
         '2019-01-26-16-03-48.bag',
         '2019-01-26-16-34-17.bag',
         '2019-01-26-17-08-36.bag',
         '2019-01-26-17-53-47.bag',
         '2019-01-26-18-08-11.bag',
         '2019-01-26-18-10-30.bag',
         '2019-01-26-20-02-31.bag',
         '2019-01-26-22-09-34.bag']

db_files = [ '8am_output1.db',
            '8am_output2.db',
            '8am_output3.db',
            '8_30am_output1.db',
            '9am_output1.db',
            '10am_output1.db',
            '10am_output2.db',
            '10am_output3.db',
            '12pm_output1.db',
            '2pm_output1.db']

print "test"
#os.system('roslaunch jet_launcher zed_rtabmap.launch ')
my_env = os.environ.copy()
my_env["PATH"] = "/opt/ros/melodic/bin:/home/jseng/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

for i in range(len(l)):
   process = subprocess.Popen(["roslaunch", "jet_launcher", "zed_rtabmap.launch"], env=my_env, close_fds=True)
   print process
   time.sleep(20)
   print process.pid
   command = 'cd /home/jseng/.ros; rosbag play --clock ' + l[i] + ';' 
   os.system(command)
   print command + ',' + db_files[i]
   time.sleep(5)
   process.terminate()
   time.sleep(10)
   copy_command = 'cd /home/jseng/.ros; cp rtabmap.db ' + db_files[i]
   os.system(copy_command)
   print copy_command

   time.sleep(10)
