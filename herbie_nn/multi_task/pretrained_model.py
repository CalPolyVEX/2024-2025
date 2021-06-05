import timm
import torch
from torchvision import models
from torchsummary import summary
import torch.nn.functional as F

class Pretrained_Model(torch.nn.Module):
# class Pretrained_Model:
    def __init__(self, shape, num_outputs1, num_outputs2, load):
        super(Pretrained_Model, self).__init__()

        self.shape = shape
        self.num_outputs1 = num_outputs1
        self.num_outputs2 = num_outputs2

        # instantiate pre-trained model
        self.m = timm.create_model('efficientnet_lite0', pretrained=True)
        #self.m = timm.create_model('hardcorenas_a', pretrained=True)
        #self.m = timm.create_model('efficientnet_b1', pretrained=True)

        self.removed = list(self.m.children())[:-1]
        self.m = torch.nn.Sequential(*self.removed)

        self.temp1 = torch.nn.Linear(1280, 80)
        self.temp2 = torch.nn.Linear(1280, 80)

        self.do = torch.nn.Dropout(p=.15)
        self.lr = torch.nn.LeakyReLU()

        self.out1 = torch.nn.Linear(80, self.num_outputs1)
        self.out2 = torch.nn.Linear(80, self.num_outputs2)

        # self.out1a = torch.nn.Linear(512, 512)
        #self.out2a = torch.nn.Linear(256, 256)

        self.softmax = torch.nn.Softmax(1)

    def forward(self, x):
        o1 = self.m(x)

        #o2 = F.relu6(self.fc0(o1), inplace=True)

        # o2 = F.relu6(self.fc1(o1), inplace=True)
        # o3 = F.relu6(self.fc1(o1), inplace=True)

        o4 = self.lr(self.temp1(o1))
        o5 = self.lr(self.temp2(o1))

        # o6a = F.relu6(self.out1a(o4))
        # o7a = F.relu6(self.out2a(o5))

        o6 = self.out1(o4)
        o7 = self.out2(o5)

        o8 = self.softmax(o7) # localization output

        return o6,o8

    def build(self):
        #self.m = timm.create_model('efficientnet_lite0', pretrained=True)

        # remove the last layer of the pretrained model
        # 'classifier' is the name of the final layer of the model
        #num_final_inputs = self.m.classifier.in_features
        # self.m.classifier[0] = torch.nn.Linear(num_final_inputs, 128)
        # self.m.out1 = torch.nn.Linear(128, self.num_outputs1)

        # self.m.classifier[1] = torch.nn.Linear(num_final_inputs, 128)
        # self.m.out2 = torch.nn.Linear(128, self.num_outputs2)

        print(self.m)
        self.print_summary()

        # return model
        return self.m

    def print_summary(self):
        if torch.cuda.is_available():
            summary(self.m.cuda(), (3, 224, 224))
        else:
            summary(self.m, (3, 224, 224))

    @staticmethod
    def set_fixed_ground(model):
        print ("--setting fixed ground--")

        # freeze everything in the model
        for param in model.parameters():
            param.requires_grad = False

        # enable training for specific layers
        model.temp1.weight.requires_grad = True
        model.temp1.bias.requires_grad = True

        model.out1.weight.requires_grad = True
        model.out1.bias.requires_grad = True

        # model.out1a.weight.requires_grad = True
        # model.out1a.bias.requires_grad = True

        print ('--backbone complete--')

        return model

    @staticmethod
    def set_fixed_localization(model):
        print ("--setting fixed localization--")

        # freeze everything in the model
        for param in model.parameters():
            param.requires_grad = False

        # enable training for specific layers
        model.temp2.weight.requires_grad = True
        model.temp2.bias.requires_grad = True

        model.out2.weight.requires_grad = True
        model.out2.bias.requires_grad = True

        # model.out2a.weight.requires_grad = True
        # model.out2a.bias.requires_grad = True

        print ('--backbone complete--')

        return model

if __name__ == '__main__':
    m = Pretrained_Model(shape=(360,640,3), num_outputs1=80, num_outputs2=64,
        load=False)
    m = m.to(device='cuda')
    # model = m.build()
    print (m)

    m.print_summary()

    x = torch.randn(1,3,360,640)
    x = x.to(device='cuda')
    outputs = m(x)
    print(outputs)
