import copy
import os, time, sys
import ground_augment
import localization_augment
import goal_augment
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

                if retrain == 1 or retrain == 2 or retrain == 3:
                    model.m.eval()
            else:
                model.eval()   # Set model to evaluate mode

            running_loss = 0.0

            # testing iterations
            ground_iterations = len(ground_dataloaders[phase])
            ground_iterator = iter(ground_dataloaders[phase])

            loc_iterations = len(loc_dataloaders[phase])
            loc_iterator = iter(loc_dataloaders[phase])

            goal_iterations = len(goal_dataloaders[phase])
            goal_iterator = iter(goal_dataloaders[phase])

            if retrain == 0 or retrain == 1:
                iterations = ground_iterations
            elif retrain == 2:
                iterations = loc_iterations
            elif retrain == 3:
                iterations = goal_iterations

            # Iterate over data.
            pbar = tqdm(total=iterations,desc=phase,ncols=70)

            for i in range(iterations):
            #for inputs, labels in dataloaders[phase]:
                loc_freq = 4
                goal_freq = 7

                if retrain == 0:
                    if i % loc_freq == 0: # localization batch
                        inputs, labels = next(loc_iterator)

                        inputs = inputs.to(device)
                        output_tensor = labels.to(device)
                    elif i % goal_freq == 0: # goal batch
                        try:
                            inputs, labels = next(goal_iterator)
                        except:
                            goal_iterator = iter(goal_dataloaders[phase])
                            inputs, labels = next(goal_iterator)

                        inputs = inputs.to(device)
                        output_tensor = labels.to(device)
                    else: # ground batch
                        inputs, labels = next(ground_iterator)

                        inputs = inputs.to(device)
                        output_tensor = labels.to(device)
                elif retrain == 1:
                    inputs, labels = next(ground_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)
                elif retrain == 2:
                    inputs, labels = next(loc_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)
                elif retrain == 3:
                    inputs, labels = next(goal_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)

                # zero the parameter gradients
                optimizer.zero_grad()

                # Run the forward pass and track history if only in training
                with torch.set_grad_enabled(phase == 'train'):
                    outputs = model(inputs)
                    preds = outputs

                    if retrain == 0:
                        if i % loc_freq == 0: #localization
                             #outputs = outputs.to(dtype=torch.long)
                             loss = 3 * criterion[1](outputs[1], output_tensor)
                        elif i % goal_freq == 0: #goal
                             loss = criterion[2](outputs[2], output_tensor)
                        else: # ground
                             loss = criterion[0](outputs[0], output_tensor)
                    elif retrain == 1:
                        loss = criterion[0](outputs[0], output_tensor)
                    elif retrain == 2:
                        loss = criterion[1](outputs[1], output_tensor)
                    elif retrain == 3:
                        loss = criterion[2](outputs[2], output_tensor)

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

            if retrain == 0 or retrain == 1:
                epoch_loss = running_loss / ground_dataset_sizes[phase]
            elif retrain == 2:
                epoch_loss = running_loss / loc_dataset_sizes[phase]
            elif retrain == 3:
                epoch_loss = running_loss / goal_dataset_sizes[phase]

            print('{} Loss: {:.4f}'.format(phase, epoch_loss))

            # deep copy the model
            if phase == 'val' and epoch_loss < best_loss:
                best_loss = epoch_loss
                best_model_wts = copy.deepcopy(model.state_dict())
                print ("Saving new best model...")
                torch.save(model, 'inference.pt')
                torch.save({
                    'epoch': epoch,
                    'model_state_dict': model.state_dict()
                    }, 'test.pt')

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
        g_workers = 10
        l_workers = 1
        go_workers = 1
    elif len(sys.argv) > 1 and sys.argv[1] == str(2):
        retrain = 2 # retrain localization
        g_workers = 1
        l_workers = 10
        go_workers = 1
    elif len(sys.argv) > 1 and sys.argv[1] == str(3):
        retrain = 3 # retrain goal
        g_workers = 1
        l_workers = 1
        go_workers = 10
    else:
        retrain = 0 # initial training
        g_workers = 4
        l_workers = 4
        go_workers = 4

    # create the dataloader for the ground dataset
    ground_train_fp, ground_val_fp = ground_augment.ground_setup_dir()
    ground_train_d, ground_val_d = ground_augment.create_datasets(ground_train_fp, ground_val_fp)

    ground_image_datasets = {}
    ground_image_datasets['train'] = ground_train_d
    ground_image_datasets['val'] = ground_val_d

    ground_dataloaders = {x: torch.utils.data.DataLoader(ground_image_datasets[x], \
                   batch_size=16, shuffle=True, num_workers=g_workers) for x in ['train', 'val']}

    ground_dataset_sizes = {x: len(ground_image_datasets[x]) for x in ['train', 'val']}

    # localization dataloader
    loc_train_fp, loc_val_fp = localization_augment.localization_setup_dir()
    loc_train_d, loc_val_d = localization_augment.create_datasets(loc_train_fp, loc_val_fp)

    loc_image_datasets = {}
    loc_image_datasets['train'] = loc_train_d
    loc_image_datasets['val'] = loc_val_d

    loc_dataloaders = {x: torch.utils.data.DataLoader(loc_image_datasets[x], \
                   batch_size=16, shuffle=True, num_workers=l_workers) for x in ['train', 'val']}

    loc_dataset_sizes = {x: len(loc_image_datasets[x]) for x in ['train', 'val']}

    # goal dataloader
    goal_train_fp, goal_val_fp = goal_augment.goal_setup_dir()
    goal_train_d, goal_val_d = goal_augment.create_datasets(goal_train_fp, goal_val_fp)

    goal_image_datasets = {}
    goal_image_datasets['train'] = goal_train_d
    goal_image_datasets['val'] = goal_val_d

    goal_dataloaders = {x: torch.utils.data.DataLoader(goal_image_datasets[x], \
        batch_size=16, shuffle=True, num_workers=go_workers) for x in ['train', 'val']}

    goal_dataset_sizes = {x: len(goal_image_datasets[x]) for x in ['train', 'val']}

    #dataset_sizes = {x: len(image_datasets[x]) for x in ['train', 'val']}
    print (ground_dataset_sizes)
    print (loc_dataset_sizes)
    print (goal_dataset_sizes)

    #create the model
    if retrain == 0:
        m = pretrained_model.Pretrained_Model(shape=(360,640,3), num_outputs1=80,
            num_outputs2=64, goal_outputs=2)
        model = m

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        optimizer = optim.AdamW(model.parameters(), lr=0.005)

    elif retrain == 1:
        # retrain the ground detection
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2)
        model = m
        print ("-----------Retraining model for ground detection--------------")

        checkpoint = torch.load('test.pt')
        model.load_state_dict(checkpoint['model_state_dict'])

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        model = pretrained_model.Pretrained_Model.set_fixed_ground(model)

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.005)

    elif retrain == 2:
        # retrain the localization head
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2)
        model = m
        print ("-----------Retraining model for localizaiton--------------")

        checkpoint = torch.load('test.pt')
        model.load_state_dict(checkpoint['model_state_dict'])

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        model = pretrained_model.Pretrained_Model.set_fixed_localization(model)

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.005)

    elif retrain == 3:
        # retrain the goal head
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2)
        model = m
        print ("-----------Retraining model for goal--------------")

        checkpoint = torch.load('test.pt')
        model.load_state_dict(checkpoint['model_state_dict'])

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        model = pretrained_model.Pretrained_Model.set_fixed_goal(model)

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.005)

    #configure the training
    criterion = [nn.L1Loss(), nn.MSELoss(), nn.L1Loss()]
    # optimizer = optim.AdamW(model.parameters(), lr=0.005)
    exp_lr_scheduler = lr_scheduler.CosineAnnealingLR(optimizer, T_max=10, verbose=True)

    #train the model
    #model = train_model(model, criterion, optimizer, exp_lr_scheduler, num_epochs=480)
    model = train_model(model, criterion, optimizer, exp_lr_scheduler, num_epochs=203)

