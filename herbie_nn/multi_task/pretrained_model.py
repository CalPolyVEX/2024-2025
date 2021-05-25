import timm
import torch
from torchvision import models
from torchsummary import summary

class Pretrained_Model:
    def __init__(self, shape, num_outputs):
        self.shape = shape
        self.num_outputs = num_outputs

    def build(self):
        # instantiate pre-trained model
        self.m = timm.create_model('efficientnet_lite0', pretrained=True)
        #self.m = timm.create_model('semnasnet_100', pretrained=True)

        # remove the last layer of the pretrained model
        # 'classifier' is the name of the final layer of the model
        num_final_inputs = self.m.classifier.in_features
        self.m.classifier = torch.nn.Linear(num_final_inputs, self.num_outputs)

        '''
        self.m.classifier = torch.nn.Sequential(
            #torch.nn.Dropout(0.1),
            #torch.nn.Linear(576, 1024),
            #torch.nn.Hardswish(),
            #torch.nn.ReLU(),
            # torch.nn.Linear(256, 256),
            # torch.nn.ReLU(),
            #torch.nn.Dropout(0.1),
            #torch.nn.Linear(num_final_inputs, self.num_outputs)
            torch.nn.Linear(1280, self.num_outputs)
        )
        '''

        print(self.m)
        self.print_summary()

        # return model
        return self.m

    def print_summary(self):
        if torch.cuda.is_available():
            summary(self.m.cuda(), (3, 224, 224))
        else:
            summary(self.m, (3, 224, 224))

if __name__ == '__main__':
    m = Pretrained_Model(shape=(360,640,3), num_outputs=64)
    model = m.build()
    print (model)

    m.print_summary()
