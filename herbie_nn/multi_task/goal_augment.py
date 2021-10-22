#generate augmented images for the goal dataset during training
#Augmented images are generated dynamically during training.

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

        if 'Training' in str(self.images_filepaths[0]):
            self.tr = 1
            # print('goal training')
        else:
            self.tr = 0
            # print('goal validation')

    def __len__(self):
        return len(self.images_filepaths)

    def __getitem__(self, idx):
        image_filepath = self.images_filepaths[idx]
        image = cv2.imread(image_filepath)

        #read in goal data
        #image_filename = image_filepath.split('/')[-1]
        image_filename = os.path.basename(image_filepath)
        #prefix = image_filename.split('.')[0]
        prefix = os.path.splitext(image_filename)[0]
        goal_data_list = prefix.split('-')
        center_x = int(goal_data_list[-3])
        center_y = int(goal_data_list[-2])

        # if self.tr == 1:
        #     print('goal training')
        # else:
        #     print('goal validation')

        if self.tr == 1:
            aug_found = 0

            #dropout part of the image
            transform_cutout = A.Compose([
                #A.CoarseDropout(max_holes=8, max_height=80, max_width=50, p=.5)
                A.CoarseDropout(max_holes=8, min_holes=5, max_height=200, max_width=50, min_height=70, p=.5),
            ])
            transformed_cutout = transform_cutout(image=image)
            image = transformed_cutout['image']

            train_transform_aug = A.Compose(
                [
                    A.RandomBrightnessContrast(p=0.5),
                    A.GaussNoise(p=.2),
                    A.ISONoise(p=.15),
                    A.RandomShadow(p=.2),
                    A.MotionBlur(p=.2),
                    A.Perspective(scale=(.05,.1),p=.5),
                    A.RandomToneCurve(p=.3)
                ],
                keypoint_params=A.KeypointParams(format='xy')
            )

            while aug_found == 0:
                keypoints = [(center_x, center_y)]
                transformed = train_transform_aug(image=image, keypoints=keypoints)
                keypoints_out = transformed['keypoints']

                #if after transformation there is a single keypoint,
                #then use the new image and keypoint
                if len(keypoints_out) == 1:
                    image = transformed['image']
                    data_list = []
                    data_list.append(float(keypoints_out[0][0])/639.0) #add the x
                    data_list.append(float(keypoints_out[0][1])/359.0) #add the y
                    aug_found = 1

            tensor_train_transform = A.Compose([
                ToTensorV2()
            ])
            transformed_train_image = tensor_train_transform(image=image)
            final_image = transformed_train_image['image']

            image = final_image / 255.0

        else:
            # add the data points to the 'data_list'
            data_list = []
            data_list.append(float(center_x) / 639.0)
            data_list.append(float(center_y) / 359.0)

            # apply the augmentations to the image
            if self.transform is not None:
                image = self.transform(image=image)["image"]
                image = image / 255.0

        d = torch.Tensor(data_list) # convert the list of data points to a tensor
        return image, d #return the image and the list of datapoints

def create_datasets(train_file_list,val_file_list):
   train_transform = A.Compose(
      [
         A.RandomBrightnessContrast(p=0.5),
         A.GaussNoise(p=.2),
         A.ISONoise(p=.15),
         A.RandomShadow(p=.2),
         A.MotionBlur(p=.2),
         #A.Perspective(scale=(.05,.1),p=.5),
         A.CoarseDropout(max_holes=8, max_height=50, max_width=50, p=.5),
         A.RandomToneCurve(p=.3),
         ToTensorV2()
      ]
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
