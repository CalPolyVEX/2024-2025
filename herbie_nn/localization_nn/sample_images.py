# This file saves heading data to a file and then runs through a .bag file
# a second time to generate heading data for a given image.  The bag file
# must publish to /see3cam_cu20/image_raw/compressed and also must publish
# the odometry to /camera/odom/sample (T265 tracking camera)
#
# To run:  python3 make_dirs.py -heading file.bag

import roslib
import rosbag
import rospy
import cv2
import sys, time, queue, os, argparse, math, shutil
import numpy as np
import glob
from sensor_msgs.msg import CompressedImage


class sample_images:
    def __init__(self, args, bagfile):
        self.image = None

        # how many frames to skip in a straight section before saving an image
        self.save_freq = 30  # save every 5 photos

        # the parent dir and the bagfile
        self.path = './Input_Images'
        self.bagfile = bagfile  # the full path to the bag file

        self.prev_dir = ""
        self.hall_num = int(args["num"])

    def clean(self):
        self.file_prefix = self.bagfile.split('/')[-1]
        self.file_prefix = self.file_prefix.split('.bag')[0]

        filelist = glob.glob(self.path + '/*.jpg')
        print (filelist)

        for f in filelist:
            if self.file_prefix in f:
                print (f)
                os.remove(f)

    def read_bag(self):
        bag = rosbag.Bag(self.bagfile)
        self.file_prefix = self.bagfile.split('/')[-1]
        self.file_prefix = self.file_prefix.split('.bag')[0]

        # check if the file directory exists
        if not os.path.exists(self.path):
            os.makedirs(self.path)
            print('Made dir ' + self.path)

        # count of images seen in a given section (may != num images saved)
        image_num = 0
        saved_image_num = 0       # counter for naming images stored
        current_dir = ""  # directory in which to store images
        image_topic = '/see3cam_cu20/image_raw/compressed'

        for topic, msg, t in bag.read_messages(topics=[image_topic]):
            if topic == image_topic:
                np_arr = np.frombuffer(msg.data, np.uint8)
                self.image = cv2.imdecode(
                    np_arr, cv2.IMREAD_COLOR)  # decode the JPG
                self.image = cv2.resize(self.image, (640, 360))  # resize

                # store the image in the current "save to" dir
                if image_num % self.save_freq == 0:
                    image_name = os.path.join(
                        self.path,
                        str(self.hall_num) + '_' + self.file_prefix + '_' + str(saved_image_num) + '.jpg')
                    cv2.imwrite(image_name, self.image)
                    print (image_name)
                    saved_image_num += 1      # increment saved image count

                # increment overall (per section though) image count
                image_num += 1

        # close the bag file
        bag.close()


def main(args):
    rospy.init_node('sample_images_node', anonymous=True)

    # create the argument parser
    parser = argparse.ArgumentParser(description='Create image directories from bag files')
    parser.add_argument('-clean', help='Clean the output directories', action='store_true')
    parser.add_argument('num', help='Hallway number', type=int)
    parser.add_argument('path', nargs='+', help='Path to 1 or more ROS bag files')

    # usage if incorrect arg num
    if len(args) < 2:
        parser.print_usage()
    else:
        # parse the arguments
        args = parser.parse_args()

        # process each bag file
        for bagfile in args.path:
            #create with argument dictionary
            dm = sample_images(args.__dict__, bagfile)

            # if '-clean' flag, just remove the output images
            if args.clean:
                dm.clean()
                continue

            # process the bag file
            dm.read_bag()


if __name__ == '__main__':
    main(sys.argv)
