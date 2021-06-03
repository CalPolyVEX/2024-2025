import copy
import os, time
import ground_augment
import localization_augment
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
            iterations = len(ground_dataloaders[phase])

            # Iterate over data.
            pbar = tqdm(total=iterations,desc=phase,ncols=70)

            # testing iterations
            ground_iterations = len(ground_dataloaders[phase])
            ground_iterator = iter(ground_dataloaders[phase])

            loc_iterations = len(loc_dataloaders[phase])
            loc_iterator = iter(loc_dataloaders[phase])

            for i in range(ground_iterations):
            #for inputs, labels in dataloaders[phase]:
                loc_freq = 2

                if i % loc_freq == 0:
                    inputs, labels = next(loc_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)
                else:
                    inputs, labels = next(ground_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)

                #print(output_tensor.shape)

                # zero the parameter gradients
                optimizer.zero_grad()

                # Run the forward pass and track history if only in training
                with torch.set_grad_enabled(phase == 'train'):
                    outputs = model(inputs)
                    preds = outputs

                    #print (outputs.shape)
                    if i % loc_freq == 0: #localization
                         #outputs = outputs.to(dtype=torch.long)
                         loss = criterion[1](outputs[1], output_tensor)
                    else: # ground
                         loss = criterion[0](outputs[0], output_tensor)

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

            epoch_loss = running_loss / ground_dataset_sizes[phase]

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
    if len(sys.argv) > 1 and sys.argv[1] == str(1):
        retrain = 1 # retrain ground
    elif len(sys.argv) > 1 and sys.argv[1] == str(2):
        retrain = 2 # retrain localization
    elif len(sys.argv) > 1 and sys.argv[1] == str(3):
        retrain = 3 # retrain goal
    else:
        retrain = 0 # initial training

    # create the dataloader for the ground dataset
    ground_train_fp, ground_val_fp = ground_augment.ground_setup_dir()
    ground_train_d, ground_val_d = ground_augment.create_datasets(ground_train_fp, ground_val_fp)

    ground_image_datasets = {}
    ground_image_datasets['train'] = ground_train_d
    ground_image_datasets['val'] = ground_val_d

    ground_dataloaders = {x: torch.utils.data.DataLoader(ground_image_datasets[x], \
                   batch_size=32, shuffle=True, num_workers=6) for x in ['train', 'val']}

    ground_dataset_sizes = {x: len(ground_image_datasets[x]) for x in ['train', 'val']}

    # localization dataloader
    loc_train_fp, loc_val_fp = localization_augment.localization_setup_dir()
    loc_train_d, loc_val_d = localization_augment.create_datasets(loc_train_fp, loc_val_fp)

    loc_image_datasets = {}
    loc_image_datasets['train'] = loc_train_d
    loc_image_datasets['val'] = loc_val_d

    loc_dataloaders = {x: torch.utils.data.DataLoader(loc_image_datasets[x], \
                   batch_size=32, shuffle=True, num_workers=6) for x in ['train', 'val']}

    loc_dataset_sizes = {x: len(loc_image_datasets[x]) for x in ['train', 'val']}

    #ground data
    # train_fp, val_fp = ground_augment.ground_setup_dir()
    # train_d, val_d = ground_augment.create_datasets(train_fp, val_fp)

    # image_datasets = {}
    # image_datasets['train'] = train_d
    # image_datasets['val'] = val_d

    # dataloaders = {x: torch.utils.data.DataLoader(image_datasets[x], \
    #                batch_size=32, shuffle=True, num_workers=12) for x in ['train', 'val']}

    #dataset_sizes = {x: len(image_datasets[x]) for x in ['train', 'val']}
    print (ground_dataset_sizes)

    #create the model
    if retrain == 0:
        m = pretrained_model.Pretrained_Model(shape=(360,640,3), num_outputs1=80,
            num_outputs2=64, goal_outputs=2)
        model = m

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        optimizer = optim.AdamW(model.parameters(), lr=0.005)
    elif retrain == 1:
        pass
    elif retrain == 2:
        pass
    # enable training for specific layers
    # model.temp2.weight.requires_grad = False
    # model.temp2.bias.requires_grad = False

    # model.out2.weight.requires_grad = False
    # model.out2.bias.requires_grad = False

    # model.temp2.weight.requires_grad = False
    # model.temp2.bias.requires_grad = False

    # model.out2.weight.requires_grad = False
    # model.out2.bias.requires_grad = False
    #model = m.build()

    # if torch.cuda.is_available(): #send the model to the GPU if available
    #     model.cuda()

    #configure the training
    criterion = [nn.L1Loss(), nn.MSELoss()]
    # optimizer = optim.AdamW(model.parameters(), lr=0.005)
    exp_lr_scheduler = lr_scheduler.CosineAnnealingLR(optimizer, T_max=10, verbose=True)

    #train the model
    #model = train_model(model, criterion, optimizer, exp_lr_scheduler, num_epochs=480)
    model = train_model(model, criterion, optimizer, exp_lr_scheduler, num_epochs=203)

