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
        self.turn_outputs = 1

        # instantiate pre-trained model for multiple output heads
        self.m = timm.create_model('efficientnet_lite0', pretrained=True)
        self.removed = list(self.m.children())[:-1]
        self.m = torch.nn.Sequential(*self.removed)

        # ground boundary backbone
        self.m1 = timm.create_model('efficientnet_lite0', pretrained=True)
        self.removed1 = list(self.m1.children())[:-1]
        self.m1 = torch.nn.Sequential(*self.removed1)

        self.r6 = torch.nn.ReLU6()
        self.lr1 = torch.nn.LeakyReLU()
        self.lr2 = torch.nn.LeakyReLU()
        self.lr3 = torch.nn.LeakyReLU()
        self.lr4 = torch.nn.LeakyReLU()
        self.lr5 = torch.nn.LeakyReLU()
        self.feature_num = 1280

        self.ground_out = torch.nn.Linear(1280, self.num_ground_outputs)

        self.loc_hidden1 = torch.nn.Linear(self.feature_num, 256)
        self.loc_hidden2 = torch.nn.Linear(256, 256)
        self.loc_bn1 = torch.nn.BatchNorm1d(num_features=256)
        self.loc_bn2 = torch.nn.BatchNorm1d(num_features=256)
        self.loc_out = torch.nn.Linear(256, self.num_loc_outputs)

        self.turn_hidden1 = torch.nn.Linear(self.feature_num, 128)
        self.turn_hidden2 = torch.nn.Linear(128, 128)
        self.turn_bn1 = torch.nn.BatchNorm1d(num_features=128)
        self.turn_bn2 = torch.nn.BatchNorm1d(num_features=128)
        self.turn_out = torch.nn.Linear(128, self.turn_outputs)

        self.goal_hidden = torch.nn.Linear(self.feature_num, 8)
        self.goal_bn1 = torch.nn.BatchNorm1d(num_features=8)
        self.goal_out = torch.nn.Linear(8, self.goal_outputs)

        self.softmax = torch.nn.Softmax(1)
        self.sig = torch.nn.Sigmoid()
        self.sig1 = torch.nn.Sigmoid()

    def forward(self, x):
        backbone_out = self.m(x)
        backbone_out1 = self.m1(x)
        #backbone_out = self.lr(self.feature_bn(self.feature_hidden(self.m(x))))
        # backbone_out = self.lr(self.feature_hidden(self.m(x)))

        # ground output
        ground_output_val = self.ground_out(backbone_out1)

        # localization output
        l_out1 = self.lr1(self.loc_bn1(self.loc_hidden1(backbone_out)))
        l_out2 = self.lr2(self.loc_bn2(self.loc_hidden2(l_out1)))
        o7 = self.loc_out(l_out2)
        loc_output = self.softmax(o7) # localization output
        #o8 = self.sig1(o7) # localization output

        # turn output
        turn_hidden_out1 = self.lr3(self.turn_bn1(self.turn_hidden1(backbone_out)))
        # turn_hidden_out1 = self.lr3(self.turn_hidden1(backbone_out))
        # turn_hidden_out2 = self.lr(self.turn_bn2(self.turn_hidden2(turn_hidden_out1)))
        turn_hidden_out2 = self.lr4(self.turn_hidden2(turn_hidden_out1))
        turn_out_val = self.sig(self.turn_out(turn_hidden_out2))

        # goal output
        goal_hidden_out = self.lr5(self.goal_bn1(self.goal_hidden(backbone_out)))
        goal_out_val = self.goal_out(goal_hidden_out)

        return ground_output_val, loc_output, turn_out_val, goal_out_val

    def build(self):
        print(self.m)
        self.print_summary()

    def print_summary(self):
        if torch.cuda.is_available():
            summary(self.m.cuda(), (3, 224, 224))
        else:
            summary(self.m, (3, 224, 224))

    @staticmethod
    def enable_ground_head(model, status):
        #disable localization head training
        model.ground_out.weight.requires_grad = status
        model.ground_out.bias.requires_grad = status

        if status == False:
            model.ground_out.eval()
        else:
            model.ground_out.train()

        return model

    @staticmethod
    def enable_loc_head(model, status):
        #disable localization head training
        model.loc_hidden1.weight.requires_grad = status
        model.loc_hidden1.bias.requires_grad = status

        model.loc_hidden2.weight.requires_grad = status
        model.loc_hidden2.bias.requires_grad = status

        model.loc_bn1.weight.requires_grad = status
        model.loc_bn1.bias.requires_grad = status

        model.loc_bn2.weight.requires_grad = status
        model.loc_bn2.bias.requires_grad = status

        model.loc_out.weight.requires_grad = status
        model.loc_out.bias.requires_grad = status

        if status == False:
            model.loc_hidden1.eval()
            model.loc_hidden2.eval()
            model.loc_bn1.eval()
            model.loc_bn2.eval()
            model.loc_out.eval()
        else:
            model.loc_hidden1.train()
            model.loc_hidden2.train()
            model.loc_bn1.train()
            model.loc_bn2.train()
            model.loc_out.train()

        return model

    @staticmethod
    def enable_turn_head(model, status):
        #disable turn head training
        model.turn_hidden1.weight.requires_grad = status
        model.turn_hidden1.bias.requires_grad = status

        model.turn_hidden2.weight.requires_grad = status
        model.turn_hidden2.bias.requires_grad = status

        model.turn_bn1.weight.requires_grad = status
        model.turn_bn1.bias.requires_grad = status

        model.turn_bn2.weight.requires_grad = status
        model.turn_bn2.bias.requires_grad = status

        model.turn_out.weight.requires_grad = status
        model.turn_out.bias.requires_grad = status

        if status == False:
            model.turn_hidden1.eval()
            model.turn_hidden2.eval()
            model.turn_bn1.eval()
            model.turn_bn2.eval()
            model.turn_out.eval()
        else:
            model.turn_hidden1.train()
            model.turn_hidden2.train()
            model.turn_bn1.train()
            model.turn_bn2.train()
            model.turn_out.train()

        return model

    @staticmethod
    def enable_goal_head(model, status):
        #disable goal head training
        model.goal_hidden.weight.requires_grad = status
        model.goal_hidden.bias.requires_grad = status

        model.goal_bn1.weight.requires_grad = status
        model.goal_bn1.bias.requires_grad = status

        model.goal_out.weight.requires_grad = status
        model.goal_out.bias.requires_grad = status

        if status == False:
            model.goal_hidden.eval()
            model.goal_bn1.eval()
            model.goal_out.eval()
        else:
            model.goal_hidden.train()
            model.goal_bn1.train()
            model.goal_out.train()

        return model

    @staticmethod
    def set_train_ground(model):
        #enable ground head
        model = Pretrained_Model.enable_ground_head(model, True)

        #disable localization head training
        model = Pretrained_Model.enable_loc_head(model, False)

        #disable turn head
        model = Pretrained_Model.enable_turn_head(model, False)

        #disable goal head
        model = Pretrained_Model.enable_goal_head(model, False)

        return model

    @staticmethod
    def set_train_loc(model):
        #enable ground head
        model = Pretrained_Model.enable_ground_head(model, False)

        #disable localization head training
        model = Pretrained_Model.enable_loc_head(model, True)

        #disable turn head
        model = Pretrained_Model.enable_turn_head(model, False)

        #disable goal head
        model = Pretrained_Model.enable_goal_head(model, False)

        return model

    @staticmethod
    def set_train_turn(model):
        #enable ground head
        model = Pretrained_Model.enable_ground_head(model, False)

        #disable localization head training
        model = Pretrained_Model.enable_loc_head(model, False)

        #disable turn head
        model = Pretrained_Model.enable_turn_head(model, True)

        #disable goal head
        model = Pretrained_Model.enable_goal_head(model, False)

        return model

    @staticmethod
    def set_train_goal(model):
        #enable ground head
        model = Pretrained_Model.enable_ground_head(model, False)

        #disable localization head training
        model = Pretrained_Model.enable_loc_head(model, False)

        #disable turn head
        model = Pretrained_Model.enable_turn_head(model, False)

        #disable goal head
        model = Pretrained_Model.enable_goal_head(model, True)

        return model

    @staticmethod
    def set_fixed_ground(model):
        print ("--setting fixed ground--")

        # freeze everything in the model
        for param in model.parameters():
            param.requires_grad = False

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

        # batch normalization layers
        model.loc_bn1.weight.requires_grad = True
        model.loc_bn1.bias.requires_grad = True

        model.loc_bn2.weight.requires_grad = True
        model.loc_bn2.bias.requires_grad = True

        model.loc_out.weight.requires_grad = True
        model.loc_out.bias.requires_grad = True

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

        # batch normalization layers
        model.goal_bn1.weight.requires_grad = True
        model.goal_bn1.bias.requires_grad = True

        model.goal_out.weight.requires_grad = True
        model.goal_out.bias.requires_grad = True

        print ('--backbone complete--')

        return model

    @staticmethod
    def set_fixed_turn(model):
        print ("--setting fixed turn--")

        # freeze everything in the model
        for param in model.parameters():
            param.requires_grad = False

        # enable training for specific layers
        model.turn_hidden1.weight.requires_grad = True
        model.turn_hidden1.bias.requires_grad = True

        model.turn_hidden2.weight.requires_grad = True
        model.turn_hidden2.bias.requires_grad = True

        # batch normalization layers
        model.turn_bn1.weight.requires_grad = True
        model.turn_bn1.bias.requires_grad = True

        # model.turn_bn2.weight.requires_grad = True
        # model.turn_bn2.bias.requires_grad = True

        model.turn_out.weight.requires_grad = True
        model.turn_out.bias.requires_grad = True

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
