#!/usr/bin/env python3
from __future__ import print_function

import roslib
import sys
import rospy
import cv2
import time
import math
import numpy as np
from sensor_msgs.msg import CompressedImage
import torch
from torchvision import transforms
from datetime import datetime
from pretrained_model import Pretrained_Model
from std_msgs.msg import Float64MultiArray
from sensor_msgs.msg import LaserScan
from sensor_msgs.msg import PointCloud2
import sensor_msgs.point_cloud2 as pcl2
import std_msgs.msg

class image_inference:
    def __init__(self, model_name="inference.pt", unlabeled=False):
        self.device = torch.device(
            "cuda:0" if torch.cuda.is_available() else "cpu")
        print(self.device)

        # load the model
        self.model = torch.load(model_name, map_location=self.device)
        self.model.eval()

        print(self.model)

        self.image_resized_pub = rospy.Publisher(
            "/test_image/image_raw/compressed", CompressedImage, queue_size=1)

        self.image_sub = rospy.Subscriber(
            "/see3cam_cu20/image_raw/compressed", CompressedImage, self.callback, queue_size=1, buff_size=10000000)

        self.data_pub = rospy.Publisher(
            '/nn_data', Float64MultiArray, queue_size=1)

        self.scan_pub = rospy.Publisher('scan', LaserScan, queue_size=50)
        self.pointcloud_pub = rospy.Publisher('point_cloud', PointCloud2, queue_size=50)

        self.preprocess = transforms.Compose([
            transforms.ToTensor(),
        ])

        self.point_list = []
        self.goal_x = 0
        self.goal_y = 0
        self.last_find_x = 0
        self.last_find_y = 0
        self.last_find_left = 0
        self.last_find_right = 0
        self.goal_list_x = [0] * 5
        self.goal_list_y = [0] * 5
        self.turn_times = 0

    def publish_pointcloud(self):
        cloud_points = [[1.0, 1.0, 0.0],[1.0, 2.0, 0.0],[2,0,0]]

        #header
        header = std_msgs.msg.Header()
        header.stamp = rospy.Time.now()
        header.frame_id = 'see3_cam'
        #create pcl from points
        scaled_polygon_pcl = pcl2.create_cloud_xyz32(header, cloud_points)

        #publish    
        self.pointcloud_pub.publish(scaled_polygon_pcl)

    def publish_laserscan(self):
        current_time = rospy.Time.now()
        num_readings = len(self.point_list)
        laser_frequency = 20

        scan = LaserScan()

        scan.header.stamp = current_time
        scan.header.frame_id = 'camera_link'
        scan.angle_min = -1.57
        scan.angle_max = 1.57
        scan.angle_increment = 3.14 / num_readings
        scan.time_increment = (1.0 / laser_frequency) / (num_readings)
        scan.range_min = 0.0
        scan.range_max = 5.0

        scan.ranges = []
        scan.intensities = []
        for i in range(0, num_readings):
            scan.ranges.append(5.0 * self.point_list[i][1] / 360.0)  # fake data

        self.scan_pub.publish(scan)

    def connect_boundary(self, points, img):
        for i in range(len(points)-1):
            x1 = points[i][0]
            y1 = points[i][1]
            x2 = points[i+1][0]
            y2 = points[i+1][1]
            cv2.line(img, (x1, y1), (x2, y2), (0, 255, 255), 2)

    def localization(self, output, img):
        font = cv2.FONT_HERSHEY_SIMPLEX
        lineType = 2
        max_index = 0
        max_val = 0
        second_index = 0
        second_val = 0

        loc_list = []
        loc_counter = 0

        for c in range(64):
            val = float(output[1].data[0][c])
            if val > .2:
                loc_list.append(val)
                loc_counter += 1

            if val > max_val:
                max_val = val
                max_index = c

        #   loc_var = np.var(loc_list)

        # turn detection
        turn_index = 0
        if loc_counter > 1:
            m = 0
            for c in range(30,64): # only search through the turn nodes
                val = float(output[1].data[0][c])
                if val > m:
                    m = val
                    turn_index = c

            self.turn_times += 1
            # cv2.putText(img, 'turn: ' + "{:.3f}".format(turn_index),
            #             (430, 110),
            #             font,
            #             .8,
            #             (255, 0, 255),
            #             lineType)
            # cv2.putText(img, 'count: ' + "{:.3f}".format(self.turn_times),
            #             (430, 140),
            #             font,
            #             .8,
            #             (255, 0, 255),
            #             lineType)
        else:
            self.turn_times = 0

        cv2.putText(img, str(max_index) + '  ' + "{:.3f}".format((max_val)),
                    (430, 50),
                    font,
                    .8,
                    (255, 0, 255),
                    lineType)

        #   cv2.putText(img,'straight: ' + "{:.3f}".format(straight_percent),
        #       (430,80),
        #       font,
        #       .8,
        #       (255,0,255),
        #       lineType)

        #   cv2.putText(img,'turn: ' + "{:.3f}".format(turn_percent),
        #       (430,110),
        #       font,
        #       .8,
        #       (255,0,255),
        #       lineType)

        turn = float(output[2].data[0][0])
        cv2.putText(img, 'turn: ' + "{:.3f}".format(turn),
                    (430, 150),
                    font,
                    .8,
                    (255, 0, 255),
                    lineType)

    def find_left_right(self,y,img):
        if y < self.goal_y:
            return

        if self.last_find_x == 0:
            start_search_x = int(self.goal_x / 8)
        else:
            start_search_x = self.last_find_x

        left = 0
        for i in range(start_search_x,0,-1):
            if self.point_list[i][1] > y:
                left = self.point_list[i][0]
                break

        if left == 0:
            left == self.last_find_left

        right = 639
        for i in range(start_search_x,80):
            if self.point_list[i][1] > y:
                right = self.point_list[i][0]
                break

        if left == 0:
            left == self.last_find_right

        distance = right - left
        self.last_find_x = int((left+right) / 2 / 8)
        self.last_find_left = left
        self.last_find_right = right

        print(y, left, right, distance)

        # draw center dot
        cv2.circle(img, (int((left+right)/2), y),
                    3, (0, 0, 255), -1)

        # horizontal line down middle
        cv2.line(img, (300, y), (340, y), (255, 255, 0), 1)

    def navigation_lines(self, img):
        # 3 foot lines in front of robot
        cv2.line(img, (212, 359), (256, 229),
                 (0, 255, 255), 2)  # left trapezoid side
        # right trapezoid side
        cv2.line(img, (428, 359), (384, 229), (0, 255, 255), 2)

        # 2 foot lines in front of robot
        cv2.line(img, (212, 359), (241, 275),
                 (255, 0, 0), 4)  # left trapezoid side
        # right trapezoid side
        cv2.line(img, (428, 359), (399, 275), (255, 0, 0), 4)

        # vertical line down middle
        cv2.line(img, (320, 0), (320, 359), (255, 255, 0), 1)

    def draw_goal(self, img, output):
        center_x = float(output[3].data[0][0])
        center_y = float(output[3].data[0][1])

        self.goal_list_x.append(center_x)
        self.goal_list_y.append(center_y)
        self.goal_list_x.pop(0)
        self.goal_list_y.pop(0)

        var_x = np.var(self.goal_list_x)
        var_y = np.var(self.goal_list_y)

        #print (var_x, var_y)

        if var_x < .0009 and var_y < .0009:
            avg_x = np.median(self.goal_list_x)
            avg_y = np.median(self.goal_list_y)
            cv2.circle(img, (int(avg_x*640), int(avg_y*360)),
                       3, (0, 255, 0), -1)
        else:
            cv2.circle(img, (int(center_x*640), int(center_y*360)),
                       3, (255, 0, 255), -1)

    def callback(self, msg_in):
        #### direct conversion to CV2 ####
        # Convert it to something opencv understands
        start_time = time.time_ns()
        np_arr = np.frombuffer(msg_in.data, np.uint8)
        image_np = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

        image_np = cv2.resize(image_np, (640, 360))  # resize
        image_np_360 = image_np

        image_np_tensor = self.preprocess(image_np)
        image_np_tensor = image_np_tensor.to(device=self.device)
        image_np_tensor = image_np_tensor.unsqueeze(0)

        # run the forward pass of the model
        with torch.no_grad():
            output = self.model(image_np_tensor)
        end_time = time.time_ns()

        # draw the ground boundary
        self.point_list = []
        for c in range(0, 80):
            cv2.circle(image_np_360, ((
                c*8), round(float(output[0].data[0][c]) * (360-1))), 1, (0, 255, 0), -1)
            self.point_list.append(
                (c*8, round(float(output[0].data[0][c]) * (360-1))))

        #find the left/right
        #self.publish_laserscan()
        self.publish_pointcloud()
        self.goal_x = int(640 * float(output[3].data[0][0]))
        self.goal_y = int(360 * float(output[3].data[0][1]))
        self.last_find_x = 0
        self.find_left_right(110, image_np_360)
        self.find_left_right(120, image_np_360)
        self.find_left_right(130, image_np_360)
        self.find_left_right(140, image_np_360)
        self.find_left_right(150, image_np_360)
        self.find_left_right(170, image_np_360)
        self.find_left_right(200, image_np_360)
        self.find_left_right(240, image_np_360)
        self.find_left_right(280, image_np_360)
        self.find_left_right(340, image_np_360)

        font = cv2.FONT_HERSHEY_SIMPLEX
        fontScale = .7
        fontColor = (255, 255, 255)
        lineType = 2
        ms_time = (end_time - start_time)/1000000.0
        fps = 1000.0/ms_time

        self.connect_boundary(self.point_list, image_np_360)
        self.localization(output, image_np_360)
        self.navigation_lines(image_np_360)

        # draw the goal circle
        self.draw_goal(image_np_360, output)

        cv2.putText(image_np_360, str(f'{ms_time:.2f}') + 'ms FPS: ' + str(fps),
                    (430, 25),
                    font,
                    fontScale,
                    fontColor,
                    lineType)

        # create the message to publish
        msg = CompressedImage()
        msg.header.stamp = rospy.Time.now()
        msg.format = "jpeg"
        msg.data = np.array(cv2.imencode('.jpg', image_np_360)[1]).tobytes()

        # create the data message
        data_to_send = Float64MultiArray()  # the data to be sent
        nn_data_list = []

        for p in range(80):  # ground boundary data
            nn_data_list.append(float(output[0].data[0][p]))
        for p in range(64):  # localization data
            nn_data_list.append(float(output[1].data[0][p]))
        for p in range(1):  # turn data
            nn_data_list.append(float(output[2].data[0][p]))
        for p in range(2):  # goal data
            nn_data_list.append(float(output[3].data[0][p]))

        data_to_send.data = nn_data_list

        self.data_pub.publish(data_to_send)

        try:
            self.image_resized_pub.publish(msg)
        except CvBridgeError as e:
            print(e)


def main(args):
    rospy.init_node('inference_node', anonymous=True)

    if len(args) == 2 and args[1] == 'unlabeled':
        # if the argument is 'unlabeled', then record images periodically
        # this is for the ground detection neural network
        ic = image_inference(unlabeled=True)
    elif len(args) == 2:
        ic = image_inference(model_name=args[1], unlabeled=False)
    else:
        ic = image_inference()

    try:
        rospy.spin()
    except KeyboardInterrupt:
        print("Shutting down")
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main(sys.argv)
