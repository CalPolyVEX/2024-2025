import timm
import torch
from torchvision import models
from torchsummary import summary
import torch.nn.functional as F

class Pretrained_Model(torch.nn.Module):
# class Pretrained_Model:
    def __init__(self, shape, num_outputs1, num_outputs2, goal_outputs, load):
        super(Pretrained_Model, self).__init__()

        self.shape = shape
        self.num_outputs1 = num_outputs1
        self.num_outputs2 = num_outputs2
        self.goal_outputs = goal_outputs

        # instantiate pre-trained model
        self.m = timm.create_model('efficientnet_lite0', pretrained=True)
        #self.m = timm.create_model('hardcorenas_a', pretrained=True)
        #self.m = timm.create_model('efficientnet_b1', pretrained=True)

        self.removed = list(self.m.children())[:-1]
        self.m = torch.nn.Sequential(*self.removed)

        self.temp1 = torch.nn.Linear(1280, 80)
        self.temp2 = torch.nn.Linear(1280, 64)
        self.temp3 = torch.nn.Linear(1280, 10)

        self.lr = torch.nn.LeakyReLU()

        #self.out1 = torch.nn.Linear(80, self.num_outputs1)
        self.out2 = torch.nn.Linear(64, self.num_outputs2)
        self.out3 = torch.nn.Linear(10, self.goal_outputs)

        self.softmax = torch.nn.Softmax(1)

    def forward(self, x):
        o1 = self.m(x)

        o4 = self.lr(self.temp1(o1))
        o5 = self.lr(self.temp2(o1))
        goal_hidden_out = self.lr(self.temp3(o1))

        #o6 = self.out1(o4)
        o7 = self.out2(o5)
        goal_out = self.out3(goal_hidden_out)

        o8 = self.softmax(o7) # localization output

        return o4,o8,goal_out

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

        # model.out1.weight.requires_grad = True
        # model.out1.bias.requires_grad = True

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

        print ('--backbone complete--')

        return model

    @staticmethod
    def set_fixed_goal(model):
        print ("--setting fixed goal--")

        # freeze everything in the model
        for param in model.parameters():
            param.requires_grad = False

        # enable training for specific layers
        model.temp3.weight.requires_grad = True
        model.temp3.bias.requires_grad = True

        model.out3.weight.requires_grad = True
        model.out3.bias.requires_grad = True

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
