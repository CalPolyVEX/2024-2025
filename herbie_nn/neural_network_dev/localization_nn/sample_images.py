# This file saves images from a ROS bag file.  The files are saved periodically and roscore
# must be running before starting the script
#
# To save images:  python sample_images.py num path
# (where num is the hallway number and path is the ROS bag file)

import roslib
import rosbag
import rospy
import cv2
import sys, time, queue, os, argparse, math, shutil
import numpy as np
import glob
import random
import shutil
from sensor_msgs.msg import CompressedImage


class sample_images:
    def __init__(self, args, bagfile, freq=25):
        self.image = None

        # how many frames to before saving an image
        self.save_freq = freq  # save every 'freq' frames

        # the parent dir and the bagfile
        self.path = './Input_Images'
        self.training_dir = './Training_Images'
        self.validation_dir = './Validation_Images'

        self.bagfile = bagfile  # the full path to the bag file

        self.prev_dir = ""
        self.hall_num = int(args["num"])

    def save_num_files(self, save_num):
        if save_num == 0:
            return

        print ('Saving ' + str(save_num) + ' random images.')

        filelist = glob.glob(self.path + '/*.jpg')
        random.shuffle(filelist)

        if save_num > len(filelist):
            save_num = len(filelist)

        savelist = filelist[0:save_num]
        delete_list = filelist[save_num:]

        for f in delete_list:
            # print (f)
            os.remove(f)

    def clean(self):
        self.file_prefix = self.bagfile.split('/')[-1]
        self.file_prefix = self.file_prefix.split('.bag')[0]

        filelist = glob.glob(self.path + '/*.jpg')
        filelist = filelist + glob.glob(self.training_dir + '/*.jpg')
        filelist = filelist + glob.glob(self.validation_dir + '/*.jpg')
        #print (filelist)

        print('Removing files from: ' + self.path + ' and ' + self.training_dir + ' and ' + self.validation_dir)

        for f in filelist:
            if self.file_prefix in f:
                #print (f)
                os.remove(f)

    def split(self):
        training_percentage = .85 #85% of images are used for training, others used for validation

        filelist = glob.glob(self.path + '/*.jpg')
        random.shuffle(filelist)
        print (filelist)

        # check if the training directory exists
        if not os.path.exists(self.training_dir):
            os.makedirs(self.training_dir)
            print('Made dir ' + self.training_dir)

        # check if the validation directory exists
        if not os.path.exists(self.validation_dir):
            os.makedirs(self.validation_dir)
            print('Made dir ' + self.validation_dir)

        #copy training images
        training_num = int(len(filelist) * training_percentage)
        for i in range(training_num):
            shutil.copy2(filelist[0], self.training_dir)
            filelist.pop(0)

        #copy validation images
        for i in range(len(filelist)):
            shutil.copy2(filelist[0], self.validation_dir)
            filelist.pop(0)

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
        image_topic = '/see3cam_cu20/image_raw_live/compressed'

        for topic, msg, timestamp in bag.read_messages(topics=[image_topic]):
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
    parser.add_argument('-split', help='Split the data into training and validation', action='store_true')
    parser.add_argument('-freq', help='How many frames to skip before saving images', type=int)
    parser.add_argument('-save_num', help='How many total frames to save', type=int)
    parser.add_argument('num', help='Hallway number', type=int)
    parser.add_argument('path', nargs='+', help='Path to 1 or more ROS bag files')

    # usage if incorrect arg num
    if len(args) < 2:
        parser.print_usage()
    else:
        # parse the arguments
        args = parser.parse_args()

        # if '-split' flag, move files to the training and validation dirs
        if args.split:
            dm = sample_images(args.__dict__, "")
            dm.split()
        else:
            freq=25 #default image frequency

            if args.freq != None:
                freq = args.freq

            print('Capturing images every ' + str(freq) + ' frames.')

            # process each bag file
            for bagfile in args.path:
                #create with argument dictionary
                dm = sample_images(args.__dict__, bagfile, freq=freq)

                # if '-clean' flag, just all input and output images
                if args.clean:
                    dm.clean()
                    continue

                # process the bag file
                dm.read_bag()

            #set how many images to save
            save_num = 0
            if args.save_num != None:
                save_num = args.save_num
            dm.save_num_files(save_num)

if __name__ == '__main__':
    main(sys.argv)
