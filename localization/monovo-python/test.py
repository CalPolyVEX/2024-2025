import roslib, rospy
import numpy as np 
import cv2
import message_filters

from visual_odometry import PinholeCamera, VisualOdometry
from cv_bridge import CvBridge, CvBridgeError
from sensor_msgs.msg import Image

class vis_odo:
   def __init__(self):
      rospy.init_node("visual_odometry")

      #subscribers
      self.scaled_image_sub = rospy.Subscriber("/see3cam_cu20/image_raw", Image, self.callback)
      #self.raw_image_filter = message_filters.Subscriber("/see3cam_cu20/image_raw", Image)
      #self.raw_image_cache = message_filters.Cache(self.raw_image_filter,2)

      #self.point_array_sub = rospy.Subscriber("point_array", ground_boundary)

      #publisher
      self.vis_odo_image_pub = rospy.Publisher("/see3cam_cu20/test_vis_odo_img",Image, queue_size=1)

      self.width = 1920
      self.height = 1080
      self.cam = PinholeCamera(self.width, self.height, 1258.513767, 1260.515476, 949.143263, 587.553871)
      self.vo = VisualOdometry(self.cam)
      self.frames = 100

      self.traj = np.zeros((600,600,3), dtype=np.uint8)
      self.counter = 0
      self.bridge = CvBridge()


   def callback(self, data):
      if self.counter < self.frames:
         #new_time = self.raw_image_cache.getLatestTime()
         #new_img_msg = self.raw_image_cache.getElemAfterTime(new_time)
         #img = self.bridge.imgmsg_to_cv2(new_img_msg, "bgr8")
         img = self.bridge.imgmsg_to_cv2(data, "bgr8")
         img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

         #img = cv2.imread('/home/xxx/datasets/KITTI_odometry_gray/00/image_0/'+str(img_id).zfill(6)+'.png', 0)

         #vo.update(img, img_id)
         self.vo.update(img, 0)

         cur_t = self.vo.cur_t
         if(self.counter > 2):
                  x, y, z = cur_t[0], cur_t[1], cur_t[2]
         else:
                  x, y, z = 0., 0., 0.
         draw_x, draw_y = int(x)+290, int(z)+90

         #compute the ground truth
         #true_x, true_y = int(vo.trueX)+290, int(vo.trueZ)+90

         cv2.circle(self.traj, (draw_x,draw_y), 1, (self.counter*255/self.frames,255-self.counter*255/self.frames,0), 1)

         #plot the ground truth
         #cv2.circle(self.traj, (true_x,true_y), 1, (0,0,255), 2)

         cv2.rectangle(self.traj, (10, 20), (600, 60), (0,0,0), -1)
         text = "Coordinates: x=%2fm y=%2fm z=%2fm"%(x,y,z)
         cv2.putText(self.traj, text, (20,40), cv2.FONT_HERSHEY_PLAIN, 1, (255,255,255), 1, 8)

         #cv2.imshow('Road facing camera', img)
         #cv2.imshow('Trajectory', self.traj)
         #cv2.waitKey(1)
         print self.counter
         self.counter += 1
      else:
         cv2.imwrite('map.png', self.traj)
         self.counter = 0

if __name__=='__main__':
   r = vis_odo()

   # spin() simply keeps python from exiting until this node is stopped
   rospy.spin()
