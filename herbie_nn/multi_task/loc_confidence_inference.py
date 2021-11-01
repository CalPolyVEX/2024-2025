#This script does the following:
#
#  1.  for each unlabeled image, run the image through all of the trained
#  models in the 'models' directory
#  2.  find the difference between the highest and second highest confidence
#  values for a localization
#  3.  sort the images by confidence

import pretrained_model
import os, sys
import numpy as np
import cv2
import torch
from torchvision import transforms
import statistics, math
import matplotlib
import matplotlib.pyplot as plt
import argparse

class compute_confidence:
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

        self.variance_file = 'variance.csv'
        self.variance_names_only_file = 'variance_names_only.csv'

    def plot_variances(self):
        matplotlib.use('TKAgg') # use the TK output
        fig = plt.figure()
        ax = fig.add_subplot(111)
        index = [x for x in range(len(self.variances))]
        variances = [x[1] for x in self.variances]
        ax.bar(index,variances)
        plt.show()

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
            # print (imagePath)

            input_image = cv2.imread(imagePath) #use cv2, so it is BGR format
            input_image = cv2.resize(input_image, (self.width, self.height))  #resize
            input_tensor = self.preprocess(input_image) #convert the image to a tensor
            input_tensor = input_tensor.unsqueeze(0) #add 0th dimension
            input_tensor = input_tensor.to(self.device) #send to the device

            output = [] #has the output x values for 1 file

            for m in self.models:
                with torch.no_grad(): # run a forward pass through the model
                    prediction = m(input_tensor)

                p_list = list(prediction) # convert tensor to a Python list
                loc = (p_list[1])[0] #the localization is the output number 1
                #print (loc)

                #conver the tensor to a list
                loc_list = []
                for l in list(loc):
                    #print (l.item())
                    loc_list.append(l.item())

                #find the most confident localization
                first_conf = 0
                highest_index = -1
                for c in range(len(loc_list)):
                    if loc_list[c] > first_conf:
                        first_conf = loc_list[c]
                        highest_index = c

                loc_list.pop(highest_index)

                #find the second most confident localization
                second_conf = 0
                second_highest_index = -1
                for c in range(len(loc_list)):
                    if loc_list[c] > second_conf:
                        second_conf = loc_list[c]
                        second_highest_index = c

                #compute the difference between the 2 highest confidences
                loc_output = first_conf - second_conf
                output.append(loc_output)

            image_variance = sum(output)
            # print(image_variance)
            self.variances.append((file,image_variance))

            output_file = '{}/{}_inference.jpg'.format(self.output_dir, file.split(".")[0])
            print (output_file)
            cv2.imwrite(output_file, input_image)

        # sort the confidences in ascending order
        print(self.variances)
        self.variances.sort(key = lambda x: x[1])

        # save the variances to a file
        variance_file = open(self.variance_file, 'w')
        variance_names_only_file = open(self.variance_names_only_file, 'w')

        for line in self.variances:
            variance_file.write('{},{}\n'.format(line[0], str(line[1])))
            variance_names_only_file.write('{} \n'.format(line[0]))
        variance_file.close()
        variance_names_only_file.close()

        #print the average variance
        var_sum = 0
        for line in self.variances:
            var_sum += float(line[1])
        average = var_sum / len(self.variances)

        print('average x variance: ' + str(average))


    def clean(self):
        if os.path.exists(self.variance_file):
            os.remove(self.variance_file)

        if os.path.exists(self.variance_names_only_file):
            os.remove(self.variance_names_only_file)

        # remove inference images
        for file in os.listdir(self.output_dir):
            os.remove(os.path.join(self.output_dir,file))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process unlabeled images.')
    parser.add_argument('-plot', help='show bar chart', action='store_true')
    parser.add_argument('-process', help='process images', action='store_true')
    parser.add_argument('-clean', help='remove inference images and variance files', action='store_true')

    if len(sys.argv) == 1: # if no arguments are given, print the help
        parser.print_help(sys.stdout)
        sys.exit(1)

    args = parser.parse_args()

    c = compute_confidence()

    if args.clean == True:
        c.clean()

    c.load_models()

    if args.process == True:
        c.process_images()

    if args.plot == True:
        c.plot_variances()
