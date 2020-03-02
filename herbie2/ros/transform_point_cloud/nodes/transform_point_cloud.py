#!/usr/bin/env python

import rospy
import tf2_ros
import tf2_py as tf2

from geometry_msgs.msg import TransformStamped
from sensor_msgs.msg import PointCloud2
from tf2_sensor_msgs.tf2_sensor_msgs import do_transform_cloud
#from transform_point_cloud.cfg import LookupTransformConfig

class TransformPointCloud:
    def __init__(self):
        self.tf_buffer = tf2_ros.Buffer(cache_time=rospy.Duration(12))
        self.tl = tf2_ros.TransformListener(self.tf_buffer)
        self.pub = rospy.Publisher("/zed_output_transformed", PointCloud2, queue_size=2)
        self.sub = rospy.Subscriber("/voxel_grid/output", PointCloud2,
        #self.sub = rospy.Subscriber("/zed/throttled_point_cloud", PointCloud2,
                                    self.point_cloud_callback, queue_size=2)
        #self.dr_server = Server(LookupTransformConfig, self.dr_callback)

    def point_cloud_callback(self, msg):
        lookup_time = msg.header.stamp + rospy.Duration(0)
        #target_frame = msg.header.frame_id if self.config.target_frame == "" else self.config.target_frame
        #source_frame = msg.header.frame_id if self.config.source_frame == "" else self.config.source_frame
        #rospy.loginfo("source frame is %s", msg.header.frame_id)
        target_frame = "zed_camera_center"
        source_frame = "zed_camera_fixed"

        #rospy.loginfo("Can transform %s", str(self.tf_buffer.can_transform(target_frame, source_frame, lookup_time)))
        #rospy.loginfo("%s", self.tf_buffer.get_frames())

        try:
            trans = self.tf_buffer.lookup_transform(target_frame, source_frame, rospy.Time(),
                                                    rospy.Duration(1.0))
            #trans = self.tf_buffer.lookup_transform_full(target_frame, lookup_time, source_frame, lookup_time, "map",
            #                                        rospy.Duration(1.0))
        except tf2.LookupException as ex:
            rospy.logwarn(str(lookup_time.to_sec()))
            rospy.logwarn(ex)
            return
        except tf2.ExtrapolationException as ex:
            rospy.logwarn(str(lookup_time.to_sec()))
            rospy.logwarn(ex)
            return
        cloud_out = do_transform_cloud(msg, trans)
        self.pub.publish(cloud_out)

if __name__ == '__main__':
    rospy.init_node('transform_point_cloud')
    transform_point_cloud = TransformPointCloud()
    rospy.spin()
