#!/usr/bin/python

#This script automates combining rtabmap database files

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

my_env = os.environ.copy()
my_env["PATH"] = "/opt/ros/melodic/bin:/home/jseng/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

if sys.argv[1] == "vacuum":
    #remove depth images from database
    command = 'cd /home/jseng/.ros; sqlite3 output.db \"UPDATE Data SET depth = NULL; \
              \" ; sqlite3 output.db \"VACUUM;\" ;'

    #remove RGB images from database
    command += 'cd /home/jseng/.ros; sqlite3 output.db \"UPDATE Data SET image = NULL; \
              \" ; sqlite3 output.db \"VACUUM;\" '
    print command
    os.system(command)
    sys.exit()
else:
    if len(sys.argv) > 1:
        #has command line arguments
        l = sys.argv[1:]
        print l
        db_files = []
        for i in range(len(l)):
            name = str(i+1) + '.db'
            db_files.append(name)
        print db_files

s = "\""
for i in range(len(l)):
   s = s + str(i+1) + '.db;'
s = s[:-1]
s = s + '\"'
print s

command = 'cd /home/jseng/.ros; rtabmap-reprocess ' + s + ' \"output.db\"'
print command

os.system(command)
