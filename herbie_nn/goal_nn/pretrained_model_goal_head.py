import timm
import torch
from torchvision import models
from torchsummary import summary

class Goal_Model:
    def __init__(self, shape, num_outputs):
        self.shape = shape
        self.num_outputs = num_outputs
        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

    def build(self):
        # load pre-trained ground detection model
        self.m = torch.load('../ground_detection/efficientnet_lite0-.0102-80out.pt')

        for param in self.m.parameters():
            param.requires_grad = False

        ##########################################################
        # save a copy of the weights and biases stored in the final layer
        current_weights = self.m.classifier.weight.data
        current_bias = self.m.classifier.bias.data

        #print(self.m)
        #self.print_summary()

        #print (current_weights.shape)
        #print (current_bias.shape)

        new_neurons = 2 #the number of new neurons to add

        # randomly initialize a tensor of weights with the size of the wanted layer
        outputw_input = torch.zeros([new_neurons, current_weights.shape[1]], requires_grad=True) #for adding 2 neurons
        outputw_input = outputw_input.to('cuda:0')
        torch.nn.init.xavier_uniform_(outputw_input, gain=torch.nn.init.calculate_gain('relu'))

        # randomly initialize a tensor with biases of the size of the wanted layer
        outputb_input = torch.zeros([2], requires_grad=True) #for adding 2 neuron biases
        outputb_input = outputb_input.to('cuda:0')
        #torch.nn.init.normal_(outputb_input) #randomly initialize biases

        # concatenate the old weights with the new weights
        new_wi = torch.cat([current_weights, outputw_input], dim=0)
        new_bias = torch.cat([current_bias, outputb_input], dim=0)
        #print (new_bias.shape)

        # create the new output layer
        self.m.classifier = torch.nn.Linear(current_weights.shape[1], \
            current_weights.shape[0]+new_neurons)

        # set the weight data to new values
        self.m.classifier.weight = torch.nn.Parameter(new_wi)
        self.m.classifier.bias = torch.nn.Parameter(new_bias)

        new_current_weights = self.m.classifier.weight.data
        new_current_bias = self.m.classifier.bias.data

        print(self.m)
        self.print_summary()

        # print (current_bias)
        # print (new_current_bias)
        # print (current_weights)
        # print (new_current_weights)

        #freeze the entire model
        # for param in self.m.parameters():
        #     param.requires_grad = False

        #set only the last goal neurons to be trainable

        # return model
        return self.m

    def print_summary(self):
        if torch.cuda.is_available():
            summary(self.m.cuda(), (3, 360, 640))
        else:
            summary(self.m, (3, 360, 640))

if __name__ == '__main__':
    m = Goal_Model(shape=(360,640,3), num_outputs=82)
    model = m.build()
    #print (model)

    # m.print_summary()
