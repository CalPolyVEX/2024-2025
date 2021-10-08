import timm
import torch
from torchvision import models
from torchsummary import summary

class Goal_Model:
    def __init__(self, shape, num_outputs):
        self.shape = shape
        self.num_outputs = num_outputs

    def build(self):
        # instantiate pre-trained model
        #self.m = timm.create_model('mobilenetv2_100', pretrained=True)
        #self.m = timm.create_model('tf_mixnet_s', pretrained=True)
        self.m = timm.create_model('semnasnet_075', pretrained=True)
        #self.m = models.mnasnet0_5(pretrained=True)

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

        #device = torch.device('cpu')
        #self.m.to(device)

        # return model
        return self.m

    def print_summary(self):
        if torch.cuda.is_available():
            summary(self.m.cuda(), (3, 224, 224))
        else:
            summary(self.m, (3, 224, 224))

if __name__ == '__main__':
    m = Goal_Model(shape=(360,640,3), num_outputs=2)
    model = m.build()
    print (model)

    m.print_summary()
