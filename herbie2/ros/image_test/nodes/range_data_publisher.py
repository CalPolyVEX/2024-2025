#!/usr/bin/env python
#this node takes data from the /point_array topic
#and publishes the data as a point cloud

import roslib, rospy
roslib.load_manifest('image_test')
import struct

from std_msgs.msg import Int32
from sensor_msgs.msg import PointCloud2, PointField
from image_test.msg import ground_boundary
from random import randint

from sensor_msgs import point_cloud2
from std_msgs.msg import Header

class range_data_node:
   def __init__(self):
      rospy.init_node("create_cloud_xyzrgb")
      self.pub = rospy.Publisher("/test_point_cloud", PointCloud2, queue_size=2)
      self.sub = rospy.Subscriber("/point_array", ground_boundary, self.callback)

   def callback(self,data):
      ground_points_x = data.point_x
      ground_points_y = data.point_y
      #g = str(ground_points_x[0])
      #rospy.loginfo("%s", ground_points_x)
      #rospy.loginfo("%s", ground_points_y)
      points = []

      r=0
      g=255
      b = 0

      y=+.5
      for i in range(len(ground_points_x)):
         y = ground_points_x[i]
         x = ground_points_y[i]
         z = 0
         pt = [x, -y, z, 0]

         #set the color of the points
         a = 255
         rgb = struct.unpack('I', struct.pack('BBBB', b, g, r, a))[0]
         pt[3] = rgb
         points.append(pt)

      fields = [PointField('x', 0, PointField.FLOAT32, 1),
               PointField('y', 4, PointField.FLOAT32, 1),
               PointField('z', 8, PointField.FLOAT32, 1),
               PointField('rgb', 12, PointField.UINT32, 1),
               # PointField('rgba', 12, PointField.UINT32, 1),
               ]

      header = Header()
      header.stamp = rospy.Time.now()
      header.frame_id = "map"
      pc2 = point_cloud2.create_cloud(header, fields, points)
      #while not rospy.is_shutdown():
      pc2.header.stamp = rospy.Time.now()
      self.pub.publish(pc2)

if __name__=='__main__':
   r = range_data_node()

   # spin() simply keeps python from exiting until this node is stopped
   rospy.spin()
