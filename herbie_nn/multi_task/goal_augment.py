from collections import defaultdict
import copy
import random
import os

import albumentations as A
from albumentations.pytorch import ToTensorV2
import cv2
import torch
import torch.backends.cudnn as cudnn
import torch.nn as nn
import torch.optim
from torch.utils.data import Dataset, DataLoader
import torchvision.models as models

cudnn.benchmark = True

def goal_setup_dir():
    root_directory = "./"

    train_directory = os.path.join(root_directory, "Goal_Training_Images")
    val_directory = os.path.join(root_directory, "Goal_Validation_Images")

    #generate all the paths of the images
    train_images_filepaths = [os.path.join(train_directory, f) for f in os.listdir(train_directory)]
    val_images_filepaths = [os.path.join(val_directory, f) for f in os.listdir(val_directory)]

    #return the lists of file image names
    return train_images_filepaths, val_images_filepaths

class GoalDataset(Dataset):
    def __init__(self, images_filepaths, transform=None):
        self.images_filepaths = images_filepaths
        self.transform = transform
        self.img_counter = 0

        if 'Training' in str(images_filepaths):
            self.tr = 1
        else:
            self.tr = 0

    def __len__(self):
        return len(self.images_filepaths)

    def __getitem__(self, idx):
        image_filepath = self.images_filepaths[idx]
        image = cv2.imread(image_filepath)

        #read in goal data
        image_filename = image_filepath.split('/')[-1]
        prefix = image_filename.split('.')[0]
        goal_data_list = prefix.split('-')
        center_y = int(goal_data_list[-2])
        center_x = int(goal_data_list[-3])
        turn_dir = int(goal_data_list[-4])

        # add the data points to the 'data_list'
        data_list = []
        data_list.append(float(center_x) / 640.0)
        data_list.append(float(center_y) / 360.0)

        # apply the augmentations to the image
        if self.tr == 1:
            #print('Training batch')
            aug_found = 0

            transform_cutout = A.Compose([
                A.CoarseDropout(max_holes=8, max_height=50, max_width=50, p=.5)
            ])
            transformed_cutout = transform_cutout(image=image)
            image = transformed_cutout['image']
            #cv2.imwrite("test" + str(self.img_counter) + '.jpg', image)
            #self.img_counter += 1
            #print ("test write")

            while aug_found == 0:
               keypoints = [(center_x, center_y)]
               transformed = self.transform(image=image, keypoints=keypoints)
               keypoints = transformed['keypoints']
               #transformed = self.transform(image=image) #try without keypoints

               #if after transformation there is a single keypoint,
               #then use the new image and keypoint
               if len(keypoints) == 1:
                  image = transformed['image']
                  data_list = []
                  #print (data_list)
                  data_list.append(float(keypoints[0][0])/640.0) #add the x
                  data_list.append(float(keypoints[0][1])/360.0) #add the y
                  aug_found = 1

                  # cv2.imwrite("test-" + str(int(keypoints[0][0])) + '-' + \
                  #    str(int(keypoints[0][1])) + '-' + str(self.img_counter) + '.jpg', image)
                  # self.img_counter += 1
                  # print ("test write")

            tensor_train_transform = A.Compose([
                ToTensorV2()
            ])
            transformed_train_image = tensor_train_transform(image=image)
            image = transformed_train_image['image']

            image_out = image / 255.0
        else:
            #print('Validation batch')
            val_transform = A.Compose([
                ToTensorV2()
            ])
            transformed_val_image = val_transform(image=image)
            image = transformed_val_image['image']
            # image = self.transform(image=image)["image"]
            image_out = image / 255.0

        d = torch.Tensor(data_list) # convert the list of data points to a tensor
        return image_out, d #return the image and the list of datapoints

def create_datasets(train_file_list,val_file_list):
   train_transform = A.Compose(
      [
         A.RandomBrightnessContrast(p=0.5),
         A.GaussNoise(p=.2),
         A.ISONoise(p=.15),
         A.RandomShadow(p=.2),
         A.MotionBlur(p=.2),
         A.Perspective(scale=(.05,.1),p=.5),
         A.RandomToneCurve(p=.3),
         # ToTensorV2(),
      ],
      keypoint_params=A.KeypointParams(format='xy')
   )
   train_dataset = GoalDataset(images_filepaths=train_file_list, transform=train_transform)

   val_transform = A.Compose(
      [
         ToTensorV2()
      ]
   )
   val_dataset = GoalDataset(images_filepaths=val_file_list, transform=val_transform)

   return train_dataset, val_dataset

if __name__ == '__main__':
    train_fp, val_fp = setup_dir()

    train_d, val_d = create_datasets(train_fp, val_fp)
    print (len(train_d), len(val_d))
