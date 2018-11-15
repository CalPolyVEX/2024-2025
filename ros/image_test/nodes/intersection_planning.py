#!/usr/bin/env python

import roslib, rospy
roslib.load_manifest('image_test')
import struct
import message_filters
import cv2, numpy as np

from cv_bridge import CvBridge, CvBridgeError
from image_test.msg import ground_boundary
from sensor_msgs.msg import Image

#from sensor_msgs import point_cloud2
from std_msgs.msg import Header

class visual_odometry:
   def __init__(self):
      rospy.init_node("visual_odometry")
      #self.pub = rospy.Publisher("/test_point_cloud", PointCloud2, queue_size=2)

      #subscribers
      self.scaled_image_sub = rospy.Subscriber("/see3cam_cu20/image_raw", Image, self.callback)
      self.raw_image_filter = message_filters.Subscriber("/see3cam_cu20/image_raw", Image)
      self.raw_image_cache = message_filters.Cache(self.raw_image_filter,5)

      self.point_array_sub = rospy.Subscriber("point_array", ground_boundary)

      #publisher
      self.vis_odo_image_pub = rospy.Publisher("/see3cam_cu20/test_vis_odo_img",Image, queue_size=1)

      self.orb = cv2.ORB_create()
      self.bridge = CvBridge()

   def callback(self,data):
      try:
         cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
      except CvBridgeError as e:
         print(e)

      #header = Header()
      #header.stamp = rospy.Time.now()
      #header.frame_id = "map"

      #find the keypoints
      kp = self.orb.detect(cv_image,None)

      #compute the ORB descriptors
      kp,des = self.orb.compute(cv_image,kp)

      #img2 = cv2.drawKeypoints(cv_image,kp,np.array([]), color=(0,255,0), flags=0)
      #self.vis_odo_image_pub.publish(self.bridge.cv2_to_imgmsg(img2, "bgr8"))

      #KLT tracking

      # params for ShiTomasi corner detection
      feature_params = dict( maxCorners = 200,
                           qualityLevel = 0.3,
                           #minDistance = 7,
                           minDistance = 5,
                           blockSize = 7 )
      # Parameters for lucas kanade optical flow
      lk_params = dict( winSize  = (15,15),
                        maxLevel = 2,
                        criteria = (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))


      # Take first frame and find corners in it
      oldest_time = self.raw_image_cache.getOldestTime()
      oldest_img_msg = self.raw_image_cache.getElemBeforeTime(oldest_time)
      oldest_cv_image = self.bridge.imgmsg_to_cv2(oldest_img_msg, "bgr8")

      old_gray = cv2.cvtColor(oldest_cv_image, cv2.COLOR_BGR2GRAY)
      p0 = cv2.goodFeaturesToTrack(old_gray, mask = None, **feature_params)

      # Create a mask image for drawing purposes
      mask = np.zeros_like(oldest_cv_image)

      new_time = self.raw_image_cache.getLatestTime()
      new_img_msg = self.raw_image_cache.getElemAfterTime(new_time)
      new_cv_image = self.bridge.imgmsg_to_cv2(new_img_msg, "bgr8")

      new_gray = cv2.cvtColor(new_cv_image, cv2.COLOR_BGR2GRAY)

      # Create some random colors
      color = np.random.randint(0,255,(200,3))

      # calculate optical flow
      p1, st, err = cv2.calcOpticalFlowPyrLK(old_gray, new_gray, p0, None, **lk_params)
      # Select good points
      good_new = p1[st==1]
      good_old = p0[st==1]
      # draw the tracks
      for i,(new,old) in enumerate(zip(good_new,good_old)):
         a,b = new.ravel()
         c,d = old.ravel()
         mask = cv2.line(mask, (a,b),(c,d), color[i].tolist(), 2)
         new_gray= cv2.circle(new_gray,(a,b),5,color[i].tolist(),-1)
      img = cv2.add(new_cv_image,mask)
      self.vis_odo_image_pub.publish(self.bridge.cv2_to_imgmsg(img, "bgr8"))

if __name__=='__main__':
   r = visual_odometry()

   # spin() simply keeps python from exiting until this node is stopped
   rospy.spin()
