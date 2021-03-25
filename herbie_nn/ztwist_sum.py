#!/usr/bin/env python3
from __future__ import print_function

#this program integrates the Z-axis turning rate over a 1 second window and
#prints out the value (positive = left turn, negative = right turn)
#
#the program also saves the heading data to a file

import roslib
import message_filters
import sys, rospy, time
import numpy as np
from nav_msgs.msg import Odometry
import math

class read_odometry:
    def __init__(self):
        self.odometry_sub = rospy.Subscriber("/camera/odom/sample",Odometry, \
            self.callback, queue_size=20)

        self.last_time = time.time_ns()
        self.counter = 0
        self.ztwist = 0.0

        self.f = open("data.txt","w+")
        self.first = 0
        self.last_yaw = 0
        self.heading_offset = 0

    def callback(self,msg_in):
        #this function is called when a new message is received

        if self.counter == 200: #odometry is published at 200Hz
            start_time = time.time_ns()
            diff = (start_time - self.last_time) / 1000000000.0
            self.last_time = start_time

            print (self.ztwist)
            self.counter = 0
            self.ztwist = 0.0

        self.counter += 1

        #sum up the turning rate over the 200 readings
        self.ztwist += msg_in.twist.twist.angular.z

        if self.counter % 2 == 0: #process every 2nd reading (100Hz)
            w = msg_in.pose.pose.orientation.w
            x = msg_in.pose.pose.orientation.x
            y = msg_in.pose.pose.orientation.y
            z = msg_in.pose.pose.orientation.z
            t3 = +2.0 * (w * z + x * y)
            t4 = +1.0 - 2.0 * (y * y + z * z)
            yaw = 57.2958 * math.atan2(t3, t4)

            #this code handles the wrap-around of the heading from +180 to -180
            if self.first == 0:
                self.first = 1
                self.heading_offset = 0
            else:
                if self.last_yaw > 175 and yaw < -175:
                    self.heading_offset += 360.0
                    #print ('-----------------1')
                elif self.last_yaw < -175 and yaw > 175:
                    self.heading_offset -= 360.0
                    #print ('-----------------2')

            self.last_yaw = yaw

            #write to a file the heading, seconds, and nanoseconds
            self.f.write(str((self.heading_offset + yaw)) + ',')
            self.f.write(str(msg_in.header.stamp.secs) + ',')
            self.f.write(str(msg_in.header.stamp.nsecs) + '\n')

def main(args):
    rospy.init_node('odometry_node', anonymous=True) #create the ROS node

    ic = read_odometry()

    try:
        rospy.spin()
    except KeyboardInterrupt:
        print("Shutting down")

if __name__ == '__main__':
    main(sys.argv)
