import timm
import torch
from torchvision import models
from torchsummary import summary
import torch.nn.functional as F

class Pretrained_Model(torch.nn.Module):
# class Pretrained_Model:
    def __init__(self, shape, num_outputs1, num_outputs2, goal_outputs):
        super(Pretrained_Model, self).__init__()

        self.shape = shape
        self.num_ground_outputs = num_outputs1
        self.num_loc_outputs = num_outputs2
        self.goal_outputs = goal_outputs
        self.percent_outputs = 2

        # instantiate pre-trained model
        self.m = timm.create_model('efficientnet_lite0', pretrained=True)

        self.removed = list(self.m.children())[:-1]
        self.m = torch.nn.Sequential(*self.removed)
        self.r6 = torch.nn.ReLU6()
        self.lr = torch.nn.LeakyReLU()

        #self.ground_hidden1 = torch.nn.Linear(1280, 128)
        #self.ground_hidden2 = torch.nn.Linear(128, 128)
        self.ground_out = torch.nn.Linear(1280, self.num_ground_outputs)

        self.loc_hidden1 = torch.nn.Linear(1280, 128)
        self.loc_hidden2 = torch.nn.Linear(128, 128)
        self.loc_out = torch.nn.Linear(128, self.num_loc_outputs)

        self.turn_hidden1 = torch.nn.Linear(1280, 32)
        #self.turn_hidden2 = torch.nn.Linear(128, 128)
        self.turn_out = torch.nn.Linear(32, self.percent_outputs)

        self.goal_hidden = torch.nn.Linear(1280, 64)
        self.goal_out = torch.nn.Linear(64, self.goal_outputs)

        self.softmax = torch.nn.Softmax(1)

    def forward(self, x):
        backbone_out = self.m(x)

        # ground output
        # gr_h_out1 = self.r6(self.ground_hidden1(backbone_out))
        #gr_h_out2 = self.r6(self.ground_hidden2(gr_h_out1))
        #o4 = self.ground_out(gr_h_out1)
        o4 = self.ground_out(backbone_out)

        # localization output
        l_out1 = self.lr(self.loc_hidden1(backbone_out))
        l_out2 = self.lr(self.loc_hidden2(l_out1))
        o7 = self.loc_out(l_out2)
        o8 = self.softmax(o7) # localization output

        # turn output
        p_hidden_out1 = self.lr(self.turn_hidden1(backbone_out))
        #p_hidden_out2 = self.r6(self.turn_hidden2(p_hidden_out1))
        p_out = self.turn_out(p_hidden_out1)

        # goal output
        goal_hidden_out = self.lr(self.goal_hidden(backbone_out))
        goal_out = self.goal_out(goal_hidden_out)

        #return o4,o8,goal_out
        return o4, o8, p_out, goal_out

    def build(self):
        print(self.m)
        self.print_summary()

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
        model.ground_out.weight.requires_grad = True
        model.ground_out.bias.requires_grad = True

        print ('--backbone complete--')

        return model

    @staticmethod
    def set_fixed_localization(model):
        print ("--setting fixed localization--")

        # freeze everything in the model
        for param in model.parameters():
            param.requires_grad = False

        # enable training for specific layers
        model.loc_hidden1.weight.requires_grad = True
        model.loc_hidden1.bias.requires_grad = True

        model.loc_hidden2.weight.requires_grad = True
        model.loc_hidden2.bias.requires_grad = True

        model.loc_out.weight.requires_grad = True
        model.loc_out.bias.requires_grad = True

        # model.percent_out.weight.requires_grad = True
        # model.percent_out.bias.requires_grad = True

        # model.percent_hidden1.weight.requires_grad = True
        # model.percent_hidden1.bias.requires_grad = True

        # model.percent_hidden2.weight.requires_grad = True
        # model.percent_hidden2.bias.requires_grad = True

        print ('--backbone complete--')

        return model

    @staticmethod
    def set_fixed_goal(model):
        print ("--setting fixed goal--")

        # freeze everything in the model
        for param in model.parameters():
            param.requires_grad = False

        # enable training for specific layers
        model.goal_hidden.weight.requires_grad = True
        model.goal_hidden.bias.requires_grad = True

        model.goal_out.weight.requires_grad = True
        model.goal_out.bias.requires_grad = True

        print ('--backbone complete--')

        return model

if __name__ == '__main__':
    m = Pretrained_Model(shape=(360,640,3), num_outputs1=80, num_outputs2=64,
        goal_outputs=2)
    m = m.to(device='cuda')
    # model = m.build()
    print (m)

    m.print_summary()

    x = torch.randn(1,3,360,640)
    x = x.to(device='cuda')
    outputs = m(x)
    print(outputs)
