#!/usr/bin/env python
#this node takes data from the /point_array topic
#and publishes the data as a point cloud

import roslib, rospy
roslib.load_manifest('image_test')
import struct

from std_msgs.msg import Int32
from image_test.msg import ground_boundary
from random import randint

from std_msgs.msg import Header
from nav_msgs.msg import Odometry
import geometry_msgs.msg
from tf2_msgs.msg import TFMessage

class range_data_node:
   def __init__(self):
      rospy.init_node("new_zed_odometry")
      self.pub = rospy.Publisher("/zed/new_odom", Odometry, queue_size=2)
      self.sub = rospy.Subscriber("/zed/odom", Odometry, self.callback)

      #self.pub = rospy.Publisher("/tf", TFMessage, queue_size=2)
      #self.sub = rospy.Subscriber("/tf", TFMessage, self.callback_tf)

   def callback_tf(self,data):
      header = Header()
      header.stamp = rospy.Time.now()
      header.frame_id = "odom"

      #fix the Z position
      data.transforms[0].transform.translation.z = 0  

      #only allow rotations around the Z axis
      data.transforms[0].transform.rotation.x = 0  
      data.transforms[0].transform.rotation.y = 0  

      #while not rospy.is_shutdown():
      #pc2.header.stamp = rospy.Time.now()
      self.pub.publish(data)

      pass

   def callback(self,data):
      #fix the Z position
      data.pose.pose.position.z = 0  
      data.pose.pose.position.y = 0  

      #only allow rotations around the Z axis
      data.pose.pose.orientation.x = 0  
      data.pose.pose.orientation.y = 0  

      header = Header()
      header.stamp = rospy.Time.now()
      header.frame_id = "map"
      #pc2 = point_cloud2.create_cloud(header, fields, points)
      #while not rospy.is_shutdown():
      #pc2.header.stamp = rospy.Time.now()
      self.pub.publish(data)

if __name__=='__main__':
   r = range_data_node()

   # spin() simply keeps python from exiting until this node is stopped
   rospy.spin()