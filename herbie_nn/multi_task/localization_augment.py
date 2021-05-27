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

def localization_setup_dir():
    root_directory = "./"

    train_directory = os.path.join(root_directory, "Localization_Training_Images")
    val_directory = os.path.join(root_directory, "Localization_Validation_Images")

    #generate all the paths of the images
    train_images_filepaths = [os.path.join(train_directory, f) for f in os.listdir(train_directory)]
    val_images_filepaths = [os.path.join(val_directory, f) for f in os.listdir(val_directory)]

    #return the lists of file image names
    return train_images_filepaths, val_images_filepaths

class LocalizationDataset(Dataset):
    def __init__(self, images_filepaths, transform=None):
        self.images_filepaths = images_filepaths
        self.transform = transform

    def __len__(self):
        return len(self.images_filepaths)

    def __getitem__(self, idx):
        #print (idx)
        image_filepath = self.images_filepaths[idx]
        image = cv2.imread(image_filepath) #read in BGR format

        # remove the leading path
        img_name_fields = image_filepath.split('/')[-1]

        # get the first number in the filename which represents
        # which node in the graph this file belongs to
        img_name_fields = img_name_fields.split('_')[0]

        # add the data points to the 'data_list'
        data_list = []
        for l in range(64):  # 64 is the number of nodes
            if l == int(img_name_fields):  # if the correct intersection
                data_list.append(float(1.0))
            else:
                data_list.append(float(0.0))

        # data_list.append(float(0.0))
        # data_list.append(float(0.0))

        # apply the augmentations to the image
        if self.transform is not None:
            image = self.transform(image=image)["image"]
            image = image / 255.0

        #print (data_list)
        d = torch.Tensor(data_list) # convert the list of data points to a tensor
        return image, d #return the image and the list of datapoints

def create_datasets(train_file_list,val_file_list):
   # apply transforms to the training dataset
   train_transform = A.Compose(
      [
         A.RandomBrightnessContrast(p=0.5),
         A.GaussNoise(p=.25),
         A.ISONoise(p=.15),
         A.RandomShadow(p=.2),
         A.MotionBlur(p=.1),
         A.CoarseDropout(max_holes=5, max_height=30, max_width=30, p=.5),
         A.Perspective(p=.3),
         ToTensorV2(),
      ]
   )
   train_dataset = LocalizationDataset(images_filepaths=train_file_list, transform=train_transform)

   val_transform = A.Compose(
      [
         ToTensorV2(),
      ]
   )
   val_dataset = LocalizationDataset(images_filepaths=val_file_list, transform=val_transform)

   return train_dataset, val_dataset

if __name__ == '__main__':
    train_fp, val_fp = localization_setup_dir()

    train_d, val_d = create_datasets(train_fp, val_fp)
    print (len(train_d), len(val_d))
