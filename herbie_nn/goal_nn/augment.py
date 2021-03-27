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

def setup_dir():
    root_directory = "../"

    train_directory = os.path.join(root_directory, "Training_Images")
    val_directory = os.path.join(root_directory, "Validation_Images")

    #generate all the paths of the images
    train_images_filepaths = [os.path.join(train_directory, f) for f in os.listdir(train_directory)]
    val_images_filepaths = [os.path.join(val_directory, f) for f in os.listdir(val_directory)]

    #return the lists of file image names
    return train_images_filepaths, val_images_filepaths

class GoalDataset(Dataset):
    def __init__(self, images_filepaths, transform=None):
        self.images_filepaths = images_filepaths
        self.transform = transform

    def __len__(self):
        return len(self.images_filepaths)

    def __getitem__(self, idx):
        image_filepath = self.images_filepaths[idx]
        image = cv2.imread(image_filepath)

        #read in goal data
        image_filename = image_filepath.split('/')[-1]
        prefix = image_filename.split('.')[0]
        goal_data_list = prefix.split('-')
        center_y = float(goal_data_list[-2]) / 360.0
        center_x = float(goal_data_list[-3]) / 640.0
        turn_dir = int(goal_data_list[-4])

        # add the data points to the 'data_list'
        data_list = []
        data_list.append(center_x)
        data_list.append(center_y)
        keypoints = [(center_x, center_y)]

        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB) #convert to RGB

        # apply the augmentations to the image
        if self.transform is not None:
            transformed = self.transform(image=image, keypoints=keypoints)
            image = transformed['image']
            keypoints = transformed['keypoints']
            print (keypoints)

        d = torch.Tensor(data_list) # convert the list of data points to a tensor
        return image, d #return the image and the list of datapoints

def create_datasets(train_file_list,val_file_list):
   train_transform = A.Compose(
      [
         A.RandomBrightnessContrast(p=0.5),
         A.GaussNoise(p=.2),
         A.ISONoise(p=.15),
         A.RandomShadow(p=.1),
         A.MotionBlur(p=.1),
         A.Normalize(mean=(0.485, 0.456, 0.406), std=(0.229, 0.224, 0.225)),
         ToTensorV2(),
      ],
      keypoint_params=A.KeypointParams(format='xy')
   )
   train_dataset = GoalDataset(images_filepaths=train_file_list, transform=train_transform)

   val_transform = A.Compose(
      [
         A.Normalize(mean=(0.485, 0.456, 0.406), std=(0.229, 0.224, 0.225)),
         ToTensorV2(),
      ]
   )
   val_dataset = GoalDataset(images_filepaths=val_file_list, transform=val_transform)

   return train_dataset, val_dataset

if __name__ == '__main__':
    train_fp, val_fp = setup_dir()

    train_d, val_d = create_datasets(train_fp, val_fp)
    print (len(train_d), len(val_d))
