#!/usr/bin/env python
#this node takes data from the /point_array topic
#and publishes the data as a point cloud

import roslib, rospy
roslib.load_manifest('image_test')
import struct
import message_filters

from image_test.msg import ground_boundary
from sensor_msgs.msg import Image

#from sensor_msgs import point_cloud2
from std_msgs.msg import Header

class intersection_data_node:
   def __init__(self):
      rospy.init_node("intersection_planner")
      #self.pub = rospy.Publisher("/test_point_cloud", PointCloud2, queue_size=2)
      self.point_sub = message_filters.Subscriber("/see3cam_cu20/image_raw_480_270", Image)
      self.point_cache = message_filters.Cache(self.point_sub,20)

   def callback(self,data):
      header = Header()
      header.stamp = rospy.Time.now()
      header.frame_id = "map"
      #self.pub.publish(pc2)

if __name__=='__main__':
   r = intersection_data_node()

   # spin() simply keeps python from exiting until this node is stopped
   rospy.spin()
