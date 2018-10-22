#!/usr/bin/env python
import rospy

from std_msgs.msg import Int32
from sensor_msgs.msg import PointCloud2
from sensor_msgs.msg import PointField
from random import randint

#define the random_number Publisher
def random_number_publisher():
   a = PointField()
   a.datatype=7 #float32
   a.count=3

   pcl = PointCloud2()
   pcl.fields=[a]
   pcl.point_step=3
   pcl.row_step=3
   pcl.data=[10,10,10]

   rospy.init_node('random_number')
   #pub=rospy.Publisher('rand_no', Int32, queue_size=10)
   pub=rospy.Publisher('rand_no', PointCloud2, queue_size=1)
   rate= rospy.Rate(1)
   while not rospy.is_shutdown():
      rospy.loginfo(pcl)
      pub.publish(pcl)
      rate.sleep()

if __name__=='__main__':
   try:
      random_number_publisher()
   except rospy.ROSInterruptException:
      pass
