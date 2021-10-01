import pretrained_model
import os, sys
import numpy as np
import cv2
import torch
from torchvision import transforms
from PIL import Image
import statistics, math
from trajectory import Trajectory

def main(imageHeight, imageWidth, numOutputs, inputPath, outputPath):

    # Instantiate the Trajectory class
    trajectory = Trajectory(numOutputs, imageWidth, imageHeight)

    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

    if not os.path.isdir(outputPath): # make the output inference image directory
        os.mkdir(outputPath)

    # if the model name is passed on the command line, for example:
    # 'python3 inference.py ref.pt'
    # then load the PyTorch model 'ref.pt'
    if len(sys.argv) > 1:
        model = torch.load(sys.argv[1],map_location=device) # load the trained model
        model.eval() #switch the model to inference mode
    else:
        m = pretrained_model.Pretrained_Model(shape=(imageHeight,imageWidth,3), num_outputs=numOutputs)
        model = m.build()
        model.eval() #switch the model to inference mode

    model.to(device) #send the model the GPU if available

    # Convert the image to (C, H, W) Tensor format and divide by 255 to
    # normalize to [0, 1.0]
    preprocess = transforms.Compose([
       transforms.ToTensor(),
       transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
       ])

    for file in os.listdir(inputPath):
        imagePath = os.path.join(inputPath, file)

        input_image = cv2.imread(imagePath) #use cv2, so it is BGR format
        input_image = cv2.resize(input_image, (640, 360))  #resize
        input_tensor = preprocess(input_image)
        input_tensor = input_tensor.unsqueeze(0) #add 0th dimension

        input_tensor = input_tensor.to(device) #send to the GPU

        with torch.no_grad(): # run a forward pass through the model
            prediction = model(input_tensor)

        p_list = prediction.tolist()[0] # convert tensor to a Python list
        # print (prediction.shape)

        # create the inference image and draw the predicted data points
        validationMaskImage = cv2.imread(imagePath)
        validationMaskImage = cv2.resize(validationMaskImage, (640, 360))  #resize
        x = 0
        for i in range(len(p_list)):
            y = int(p_list[i] * imageHeight)
            validationMaskImage = cv2.circle(validationMaskImage, (x, y), 1, (0, 255, 0), -1)
            x += imageWidth // numOutputs
        
        # Draw the trajectory that the robot should take
        if len(sys.argv) == 3 and sys.argv[2] == '-trajectory':
            bottomCenter = ((imageWidth - 1) // 2, imageHeight - 1)
            # Target point of the trajectory
            endPoint = trajectory.get_target_point(p_list)
            cv2.arrowedLine(validationMaskImage, bottomCenter, endPoint, (0, 0, 255), 2)
            # Angle to the end point from the center
            angle = trajectory.get_angle(endPoint)
            cv2.putText(validationMaskImage, '{} degrees'.format(angle), (bottomCenter[0] + 10, bottomCenter[1] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 0, 255), 2)
            # Outline of the robot 3 feet in front
            outline = trajectory.get_outline()
            cv2.line(validationMaskImage, outline[0], outline[1], (255, 0, 0), 1)
            cv2.line(validationMaskImage, outline[1], outline[3], (255, 0, 0), 1)
            cv2.line(validationMaskImage, outline[2], outline[0], (255, 0, 0), 1)

        cv2.imwrite('{}/{}_inference.jpg'.format(outputPath, file.split('.')[0]), validationMaskImage)

def compute_variance(imageHeight, imageWidth, numOutputs, inputPath, outputPath):
    if not os.path.isdir(outputPath):
        os.mkdir(outputPath)

    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

    # Load each model in the 'models' directory
    models = []
    for model_file in os.listdir('models'):
        model = torch.load(os.path.join('models', model_file),map_location=device)
        model.eval()
        model.to(device)
        models.append(model)

    preprocess = transforms.Compose([
       transforms.ToTensor(),
       transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
       ])

    image_variances = []

    # For each input file
    for file in os.listdir(inputPath):
        imagePath = os.path.join(inputPath, file)
        img = cv2.imread(imagePath)
        img = cv2.resize(img, (640, 360))  #resize
        input_tensor = preprocess(img).unsqueeze(0).to(device)

        image_variance = 0
        points = []
        for i in range(numOutputs):
            points.append([])
        first_model_p_list = None

        # For each model, add the output to the points list
        for model in models:
            with torch.no_grad():
                p_list = model(input_tensor).tolist()[0]

            # Save the output from the first model for visualizing the prediction
            if first_model_p_list == None:
                first_model_p_list = p_list.copy()

            for i in range(len(p_list)):
                points[i].append(p_list[i])

        # Create the inference image and draw the predicted data points for the first model
        validationMaskImage = cv2.imread(imagePath)
        validationMaskImage = cv2.resize(validationMaskImage, (640, 360))  #resize
        x = 0
        for i in range(len(p_list)):
            y = int(p_list[i] * imageHeight)
            validationMaskImage = cv2.circle(validationMaskImage, (x, y), 1, (0, 255, 0), -1)
            x += imageWidth // numOutputs
        cv2.imwrite('{}/{}_inference.jpg'.format(outputPath, file.split('.')[0]), validationMaskImage)

        # Compute the variance at each output point and sum them together
        for output in points:
            image_variance += statistics.variance(output)
        image_variances.append((file, image_variance))

    # Sort the variances in ascending order
    image_variances.sort(key = lambda x: x[1])

    variance_file = open('variance.csv', 'w')
    for line in image_variances:
        variance_file.write('{},{}\n'.format(line[0], str(line[1])))
    variance_file.close()

if __name__ == '__main__':
    if len(sys.argv) == 2 and sys.argv[1] == '-variance':
        compute_variance(360, 640, 80, '../Unlabeled', 'Inference_Images')
    else:
        #main(360, 640, 128, '../Validation_Images', 'Inference_Images')
        main(360, 640, 80, '../Validation_Images', 'Inference_Images')
