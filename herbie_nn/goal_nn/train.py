import copy
import os, time
import augment
import pretrained_model

import albumentations as A
from albumentations.pytorch import ToTensorV2
import cv2
import torch
import torch.backends.cudnn as cudnn
import torch.nn as nn
import torch.optim as optim
from torch.optim import lr_scheduler
from torch.utils.data import Dataset, DataLoader
import torchvision.models as models
from tqdm import tqdm
from time import sleep

cudnn.benchmark = True

device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
#device = torch.device("cpu")

def train_model(model, criterion, optimizer, scheduler, num_epochs=25):
    since = time.time() # get the starting time of training

    best_loss = 100000.0
    tmax_factor = 1.5 # with warm restart, multiply the max factor by this amount
    tmax = 10 # after tmax iterations, the learning rate is reset

    for epoch in range(num_epochs):
        print('Epoch {}/{}'.format(epoch, num_epochs - 1))
        print('-' * 10)

        # Each epoch has a training and validation phase
        for phase in ['train', 'val']:
            if phase == 'train':
                model.train()  # Set model to training mode
            else:
                model.eval()   # Set model to evaluate mode

            running_loss = 0.0
            iterations = len(dataloaders[phase])

            # Iterate over data.
            pbar = tqdm(total=iterations,desc=phase,ncols=70)
            for inputs, labels in dataloaders[phase]:
                inputs = inputs.to(device)
                output_tensor = labels.to(device)

                # zero the parameter gradients
                optimizer.zero_grad()

                # Run the forward pass and track history if only in training
                with torch.set_grad_enabled(phase == 'train'):
                    outputs = model(inputs)
                    preds = outputs
                    loss = criterion(outputs, output_tensor)

                    # backward + optimize only if in training phase
                    if phase == 'train':
                        loss.backward()
                        optimizer.step()

                # statistics
                running_loss += loss.item() * inputs.size(0)
                pbar.update(1)
                sleep(0.01) #delay to print stats
            pbar.close()

            if phase == 'train': #adjust the learning rate if training
                scheduler.step()

            # With the cosine annealing scheduler, the learning rate is
            # gradually reduced.  If the learning rate is very small, then
            # reset the scheduler to do a warm restart
            if optimizer.param_groups[0]['lr'] < .0000001:
                tmax = int(tmax * tmax_factor)
                scheduler = lr_scheduler.CosineAnnealingLR(optimizer, T_max=tmax, verbose=True)
                print ("New T_max: " + str(tmax))

            epoch_loss = running_loss / dataset_sizes[phase]

            print('{} Loss: {:.4f}'.format(phase, epoch_loss))

            # deep copy the model
            if phase == 'val' and epoch_loss < best_loss:
                best_loss = epoch_loss
                best_model_wts = copy.deepcopy(model.state_dict())
                print ("Saving new best model...")
                torch.save(model, 'test.pt')

        print()

    time_elapsed = time.time() - since
    print('Training complete in {:.0f}m {:.0f}s'.format(
        time_elapsed // 60, time_elapsed % 60))
    print('Best val Acc: {:4f}'.format(best_loss))

    # load best model weights
    model.load_state_dict(best_model_wts)
    return model

if __name__ == '__main__':
    train_fp, val_fp = augment.setup_dir()
    train_d, val_d = augment.create_datasets(train_fp, val_fp)

    image_datasets = {}
    image_datasets['train'] = train_d
    image_datasets['val'] = val_d

    dataloaders = {x: torch.utils.data.DataLoader(image_datasets[x], \
                   batch_size=32, shuffle=True, num_workers=12) for x in ['train', 'val']}

    dataset_sizes = {x: len(image_datasets[x]) for x in ['train', 'val']}
    print (dataset_sizes)

    #create the model
    m = pretrained_model.Goal_Model(shape=(360,640,3), num_outputs=2)
    model = m.build()

    # if torch.cuda.is_available(): #send the model to the GPU if available
    #     model.cuda()

    #configure the training
    criterion = nn.L1Loss()
    optimizer = optim.AdamW(model.parameters(), lr=0.005)
    exp_lr_scheduler = lr_scheduler.CosineAnnealingLR(optimizer, T_max=10, verbose=True)

    #train the model
    model = train_model(model, criterion, optimizer, exp_lr_scheduler, num_epochs=480)

