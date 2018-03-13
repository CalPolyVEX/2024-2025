#from __future__ import print_function
import argparse
import pandas as pd
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torchvision import datasets, transforms
from torch.autograd import Variable
from torchvision.datasets import ImageFolder
from torchvision.transforms import ToTensor
from torch.utils.data import Dataset, DataLoader
from skimage import io,transform
from augment_images import ImageAugmentor
import Augmentor
import sys, os

# Training settings
parser = argparse.ArgumentParser(description='PyTorch MNIST Example')
parser.add_argument('--batch-size', type=int, default=32, metavar='N',
                    help='input batch size for training (default: 64)')
parser.add_argument('--test-batch-size', type=int, default=1000, metavar='N',
                    help='input batch size for testing (default: 1000)')
parser.add_argument('--epochs', type=int, default=200, metavar='N',
                    help='number of epochs to train (default: 10)')
parser.add_argument('--lr', type=float, default=0.01, metavar='LR',
                    help='learning rate (default: 0.01)')
parser.add_argument('--momentum', type=float, default=0.5, metavar='M',
                    help='SGD momentum (default: 0.5)')
parser.add_argument('--no-cuda', action='store_true', default=False,
                    help='disables CUDA training')
parser.add_argument('--seed', type=int, default=1, metavar='S',
                    help='random seed (default: 1)')
parser.add_argument('--log-interval', type=int, default=10, metavar='N',
                    help='how many batches to wait before logging training status')
args = parser.parse_args()
args.cuda = not args.no_cuda and torch.cuda.is_available()

torch.manual_seed(args.seed)
if args.cuda:
    torch.cuda.manual_seed(args.seed)

#kwargs = {'num_workers': 1, 'pin_memory': True} if args.cuda else {}
#train_loader = torch.utils.data.DataLoader(
#    datasets.MNIST('../data', train=True, download=True,
#                   transform=transforms.Compose([
#                       transforms.ToTensor(),
#                       transforms.Normalize((0.1307,), (0.3081,))
#                   ])),
#    batch_size=args.batch_size, shuffle=True, **kwargs)
#test_loader = torch.utils.data.DataLoader(
#    datasets.MNIST('../data', train=False, transform=transforms.Compose([
#                       transforms.ToTensor(),
#                       transforms.Normalize((0.1307,), (0.3081,))
#                   ])),
#    batch_size=args.test_batch_size, shuffle=True, **kwargs)
class ToTensor(object):
   """Convert ndarrays in sample to Tensors."""
   def __call__(self, sample):
      image, points = sample[0], sample[1]

      # swap color axis because
      # numpy image: H x W x C
      # torch image: C X H X W
      image = image.transpose((2, 0, 1))
      return (torch.from_numpy(image), torch.from_numpy(points))

class TestDataset(Dataset):
   def __init__(self, root_dir, transform=None):
      self.image_list = os.listdir(root_dir + "/320_images")
      self.root_dir = root_dir
      self.transform = transform
      self.point_list = []
      #self.new_list = []
      self.p = Augmentor.Pipeline()
      self.p.rotate(probability=1, max_left_rotation=5, max_right_rotation=5)
      self.p.flip_left_right(probability=0.5)
      self.p.zoom_random(probability=0.5, percentage_area=0.8)

      for x in range(len(self.image_list)):
      #for x in range(13000):
         i_name = self.image_list[x]
         text_x = i_name.replace('jpg','txt')
         text_x = text_x.replace('320_','320_gt_')
         x_list = pd.read_csv(root_dir + '/320_data/' + text_x, header=None)
         x_list = x_list.as_matrix()
         x_list = x_list.astype('float32')[:,-1]
         x_list = x_list / 240.0
         self.point_list.append(x_list)
         #self.new_list.append(i_name)

   def __len__(self):
      return len(self.image_list)

   def __getitem__(self, idx):
      img_name = os.path.join(self.root_dir + '/320_images', self.image_list[idx])
      image = io.imread(img_name)
      image = image.astype('float32')
      image = image / 255.0
      points = self.point_list[idx]
      sample = (image, points)

      if self.transform:
         sample = self.transform(sample)

      return sample

   def __getitem1__(self, idx):
      img_name = os.path.join(self.root_dir + '/320_images', self.image_list[idx])
      image = io.imread(img_name)
      image = image.astype('float32')
      image = image / 255.0
      points = self.point_list[idx]
      sample = (image, points)

      if self.transform:
         sample = self.transform(sample)

      return sample

t_dataset = TestDataset(root_dir='../ground_detection/gpu_code', 
                        transform=transforms.Compose([ToTensor()]))
sample = t_dataset[0]
#print sample[0]
#print sample[1]

kwargs = {'num_workers': 1, 'pin_memory': True} if args.cuda else {}
train_loader = torch.utils.data.DataLoader(
   t_dataset,
   batch_size=args.batch_size, shuffle=True, **kwargs)
#sys.exit(0)

class Net(nn.Module):
   def __init__(self):
      super(Net, self).__init__()
      self.conv1 = nn.Conv2d(3, 64, kernel_size=5, stride=2)
      self.conv2 = nn.Conv2d(64, 96, kernel_size=5)
      self.conv3 = nn.Conv2d(96, 128, kernel_size=3)
      self.conv4 = nn.Conv2d(128, 192, kernel_size=3)
      self.conv2_drop = nn.Dropout2d(p=.35)
      self.bn1 = nn.BatchNorm2d(64)
      self.bn2 = nn.BatchNorm2d(96)
      self.bn3 = nn.BatchNorm2d(128)
      self.bn4 = nn.BatchNorm2d(192)

      self.fc1 = nn.Linear(192, 96)
      self.fc2 = nn.Linear(96, 96)
      #self.fc3 = nn.Linear(96, 32)
      self.fc3 = nn.Linear(3360, 32)

   def forward(self, x):
      x = F.relu(F.max_pool2d(self.conv1(x), 2))
      x = self.bn1(x)
      x = self.conv2_drop(x)
      x = F.relu(F.max_pool2d(self.conv2_drop(self.conv2(x)), 2))
      x = self.bn2(x)
      x = F.relu(F.max_pool2d(self.conv3(x), 2))
      x = self.bn3(x)
      x = self.conv2_drop(x)
      x = F.relu(F.max_pool2d(self.conv4(x), 2))
      x = self.bn4(x)
      #print x.shape
      x = torch.transpose(x,1,2)
      x = torch.transpose(x,2,3)
      #x = x.view(x.size(0), -1)
      #print x.shape
      x = F.relu(self.fc1(x))
      #print x.shape
      x = F.dropout(x, training=self.training)
      x = F.relu(self.fc2(x))
      #print x.shape
      x = x.view(x.size(0), -1)
      x = self.fc3(x)
      return x

model = Net()
if args.cuda:
   model.cuda()

print(model)
for parameter in model.parameters():
    print(parameter)

#optimizer = optim.SGD(model.parameters(), lr=args.lr, momentum=args.momentum)
optimizer = optim.Adam(model.parameters())

def train(epoch):
   model.train()
   for batch_idx, (data, target) in enumerate(train_loader):
      if args.cuda:
         #print data
         #print target
         data, target = data.cuda(), target.cuda()
      data, target = Variable(data), Variable(target)
#      print '-------Data--------'
#      print data

#      print '-------Target--------'
#      print target
      optimizer.zero_grad()
      output = model(data)
      #del data
      #print output
      loss = F.l1_loss(output, target)
      del output
      loss.backward()
      optimizer.step()
      if batch_idx % args.log_interval == 0:
         print('Train Epoch: {} [{}/{} ({:.0f}%)]\tLoss: {:.6f}'.format(
               epoch, batch_idx * len(data), len(train_loader.dataset),
               100. * batch_idx / len(train_loader), loss.data[0]))

def test():
   model.eval()
   test_loss = 0
   correct = 0
   for data, target in test_loader:
      if args.cuda:
         data, target = data.cuda(), target.cuda()
      data, target = Variable(data, volatile=True), Variable(target)
      output = model(data)
      test_loss += F.nll_loss(output, target, size_average=False).data[0] # sum up batch loss
      pred = output.data.max(1, keepdim=True)[1] # get the index of the max log-probability
      correct += pred.eq(target.data.view_as(pred)).long().cpu().sum()

   test_loss /= len(test_loader.dataset)
   print('\nTest set: Average loss: {:.4f}, Accuracy: {}/{} ({:.0f}%)\n'.format(
      test_loss, correct, len(test_loader.dataset),
      100. * correct / len(test_loader.dataset)))

for epoch in range(1, args.epochs + 1):
   train(epoch)
   #test()
