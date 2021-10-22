#This file is for labeling goal coordinates on images.
#
#To run:  python collect_goal_ui_manual.py
#
#Keys:
#  q - quit
#  a - skip this image
#  c - no goals are visible
#  s - save the current goal to disk

import sys, cv2, time, queue, os, math, glob, random, shutil
import numpy as np
from threading import Lock
import argparse

class image_tracker:
    def __init__(self):
        self.input_path = "./Input_Images"
        self.training_path = "./Training_Images"
        self.validation_path = "./Validation_Images"
        self.validation_percentage = .15  # percentage of images reserved for validation
        self.counter = 0
        self.filelist = [f for f in glob.glob(os.path.join(self.input_path,"*.jpg"))]
        self.filelist.sort()

        self.mouseX = 0
        self.mouseY = 0
        self.key = 0
        self.l = Lock()
        self.width_threshold = 200

    def mouse_click(self,event,x,y,flags,param):
        if event == cv2.EVENT_LBUTTONDOWN:
            self.mouseX, self.mouseY = x,y
            self.target_set = 1

            print ((x,y))
            self.coord[-3] = str(self.mouseX)
            self.coord[-2] = str(self.mouseY)
            self.new_filename = '-'
            self.new_filename = self.new_filename.join(self.coord) + '.jpg' #combine the filename fields together
            print (self.new_filename)

            cv2.circle(self.img1, (int(self.coord[-3]), int(self.coord[-2])), 2, (0,255,255), -1)
            cv2.imshow('image',self.img1)

    def process_images(self):
        cv2.namedWindow('image',cv2.WINDOW_NORMAL)
        cv2.setMouseCallback('image',self.mouse_click)
        counter = 0

        for f in self.filelist:
            self.new_filename = f
            print (counter)
            self.img1 = cv2.imread(f)
            self.orig_img = cv2.imread(f)
            coord = f.split('.jpg')   #remove .jpg extension
            self.coord = coord[0].split('-')

            cv2.circle(self.img1, (int(self.coord[-3]), int(self.coord[-2])), 2, (0,255,255), -1)

            #draw vertical lines
            cv2.line(self.img1, (320-self.width_threshold, 0), (320-self.width_threshold, 359), (0, 255, 255), 1)
            cv2.line(self.img1, (320+self.width_threshold, 0), (320+self.width_threshold, 359), (0, 255, 255), 1)

            cv2.imshow('image',self.img1)

            while True:
                key = cv2.waitKey(0)

                if key == ord('q'):
                    cv2.destroyAllWindows()
                    return
                elif key == ord('n'):
                    break
                elif key == ord('s'): #save the new file and delete the old file
                    print("old file: " + f)
                    print("new file: " + self.new_filename)
                    os.remove(f)
                    cv2.imwrite(self.new_filename,self.orig_img)
                    break
                elif key == ord('c'): #set the goal to the upper center (no goal state)
                    self.mouse_click(cv2.EVENT_LBUTTONDOWN,320,0,0,0)
                elif key == ord('a'): #skip the image
                    break

        counter += 1

    def split_imgs(self):
        source_filelist = [f for f in glob.glob(os.path.join(self.input_path,"*.jpg"))]
        random.shuffle(source_filelist)
        print(source_filelist)
        val_count = int(len(source_filelist)*self.validation_percentage)

        val_list = source_filelist[0:val_count] #build the list of validation images
        train_list = source_filelist[val_count:] #build the training images list

        print('Number of training images: ' + str(len(train_list)))
        print('Number of validation images: ' + str(len(val_list)))

        for f in val_list:
            new_val_file = os.path.join(self.validation_path,os.path.basename(f))
            print(f, new_val_file)
            shutil.copy(f, new_val_file)

        for f in train_list:
            new_train_file = os.path.join(self.training_path,os.path.basename(f))
            print(f, new_train_file)
            shutil.copy(f, new_train_file)

    def clean(self):
        # remove training images
        for file in os.listdir(self.training_path):
            os.remove(os.path.join(self.training_path,file))

        # remove validation images
        for file in os.listdir(self.validation_path):
            os.remove(os.path.join(self.validation_path,file))

        print('Files deleted from ' + self.training_path + ' and ' + self.validation_path)

def main(args):
    parser = argparse.ArgumentParser(description='Label images for the goal neural network.')
    parser.add_argument('-clean', help='remove Training and Validation images', action='store_true')
    parser.add_argument('-split', help='split images into Training and Validation', action='store_true')
    parser.add_argument('-label', help='label images', action='store_true')

    if len(sys.argv) == 1: # if no arguments are given, print the help
        parser.print_help(sys.stdout)
        sys.exit(1)

    args = parser.parse_args()
    ic = image_tracker()

    if args.clean == True:
        ic.clean()
        sys.exit()

    if args.split == True:
        ic.split_imgs()
        sys.exit()

    ic.process_images()

    cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
