#This script does the following:
#
#  1.  for each unlabeled image, run the image through all of the trained
#  models in the 'models' directory
#  2.  compute the variance of the output for each image
#  3.  sort the images by variance

import pretrained_model
import os, sys
import numpy as np
import cv2
import torch
from torchvision import transforms
from PIL import Image
import statistics, math

class compute_variance:
    def __init__(self, model_dir='models', unlabeled_dir='unlabeled_images'):
        self.model_dir = model_dir
        self.unlabeled_dir = unlabeled_dir
        self.output_dir = 'inference_images'
        self.width = 640
        self.height = 360

        #use CUDA if available, otherwise use the CPU
        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

        # Convert the image to (C, H, W) Tensor format and normalize pixel
        # values in the range [0,1]
        self.preprocess = transforms.Compose([
            transforms.ToTensor()
        ])

        self.models = []    #list of trained models
        self.variances = [] #list of variances for all files

    def load_models(self):
        #load each of the models in the model directory
        for m in os.listdir(self.model_dir):
            model = torch.load(os.path.join(self.model_dir,m),map_location=self.device) # load the trained model
            model.to(self.device) #send the model to the device
            model.eval() #switch the model to inference mode

            self.models.append(model)

    def process_images(self):
        # make the output inference image directory
        if not os.path.isdir(self.output_dir):
            os.mkdir(self.output_dir)

        # for each of the unlabeled images, run it through all models
        for file in os.listdir(self.unlabeled_dir):
            imagePath = os.path.join(self.unlabeled_dir, file)
            print (imagePath)

            input_image = cv2.imread(imagePath) #use cv2, so it is BGR format
            input_image = cv2.resize(input_image, (640, 360))  #resize
            input_tensor = self.preprocess(input_image) #convert the image to a tensor
            input_tensor = input_tensor.unsqueeze(0) #add 0th dimension
            input_tensor = input_tensor.to(self.device) #send to the device

            variance = [] #has the output values for 1 file
            for m in self.models:
                with torch.no_grad(): # run a forward pass through the model
                    prediction = m(input_tensor)

                p_list = list(prediction) # convert tensor to a Python list
                goal = (p_list[3])[0] #the goal is the output number 3

                goal_x = int((goal[0]).item())
                goal_y = int((goal[1]).item())
                # print (goal_x)
                # print (goal_y)
                cv2.circle(input_image, (goal_x, goal_y), 4, [255,255,0], -1)
                variance.append(goal_x)

            image_variance = statistics.variance(variance)
            print(image_variance)
            self.variances.append((file,image_variance))

            output_file = '{}/{}_inference.jpg'.format(self.output_dir, file.split(".")[0])
            print (output_file)
            cv2.imwrite(output_file, input_image)

        # Sort the variances in ascending order
        print(self.variances)
        self.variances.sort(key = lambda x: x[1])

        variance_file = open('variance.csv', 'w')
        for line in self.variances:
            variance_file.write('{},{}\n'.format(line[0], str(line[1])))
        variance_file.close()

if __name__ == '__main__':
    c = compute_variance()
    c.load_models()
    c.process_images()
