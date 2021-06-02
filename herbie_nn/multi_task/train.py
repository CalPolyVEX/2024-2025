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

def train_model(model, criterion1, criterion2, criterion3, opt, scheduler, num_epochs=25):
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
                #model.apply(deactivate_batchnorm)
                model.train()  # Set model to training mode

                if retrain == 1 or retrain == 2 or retrain == 3:
                    model.m.eval()
            else:
                #model.apply(deactivate_batchnorm)
                model.eval()   # Set model to evaluate mode
                model.m.eval()

            ground_iterations = len(ground_dataloaders[phase])
            localization_iterations = len(localization_dataloaders[phase])
            goal_iterations = len(goal_dataloaders[phase])

            # number of iterations determined the larger dataset
            if retrain == 0:
               if localization_iterations > ground_iterations:
                  iterations = int(localization_iterations * 0.7)
               else:
                  iterations = ground_iterations
            elif retrain == 1:
                iterations = ground_iterations
            elif retrain == 2:
                iterations = localization_iterations
            elif retrain == 3:
                iterations = goal_iterations

            #print ('iterations: ' + str(iterations))

            running_loss = 0.0
            #iterations = len(ground_dataloaders[phase])
            #iterations = l

            ground_iterator = iter(ground_dataloaders[phase])
            localization_iterator = iter(localization_dataloaders[phase])
            goal_iterator = iter(goal_dataloaders[phase])

            # Iterate over data.
            pbar = tqdm(total=iterations,desc=phase,ncols=70)
            #for inputs, labels in ground_dataloaders[phase]:
            for i in range(iterations):
                if retrain == 0 or retrain == 1:
                    try:
                        gnd_inputs,gnd_labels = next(ground_iterator)
                    except:
                        ground_iterator = iter(ground_dataloaders[phase])
                        gnd_inputs,gnd_labels = next(ground_iterator)

                    gnd_inputs = gnd_inputs.to(device)
                    gnd_output_tensor = gnd_labels.to(device)

                if retrain == 0 or retrain == 2:
                    try:
                        loc_inputs,loc_labels = next(localization_iterator)
                    except:
                        localization_iterator = iter(localization_dataloaders[phase])
                        loc_inputs,loc_labels = next(localization_iterator)

                    loc_inputs = loc_inputs.to(device)
                    loc_output_tensor = loc_labels.to(device)

                #if retrain == 0 or retrain == 3:
                if retrain == 3:
                    try:
                        goal_inputs,goal_labels = next(goal_iterator)
                    except:
                        goal_iterator = iter(goal_dataloaders[phase])
                        goal_inputs,goal_labels = next(goal_iterator)

                    goal_inputs = goal_inputs.to(device)
                    goal_output_tensor = goal_labels.to(device)

                # zero the parameter gradients
                opt.zero_grad()

                # Run the forward pass and track history if only in training
                with torch.set_grad_enabled(phase == 'train'):
                    if retrain == 0 or retrain == 1:
                        outputs = model(gnd_inputs)
                        ground_output = outputs[0] # get the first 80 outputs
                        loss1 = criterion1(ground_output, gnd_output_tensor)

                    if retrain == 0 or retrain == 2:
                        output2 = model(loc_inputs)
                        loc_output = output2[1] # get the last 66 outputs
                        loss2 = criterion2(loc_output, loc_output_tensor)

                    #if retrain == 0 or retrain == 3:
                    if retrain == 3:
                        # print ('shape')
                        # print (goal_inputs.shape)
                        outputs = model(goal_inputs)
                        goal_output = outputs[2] # get the last 2 goal outputs
                        loss3 = criterion3(goal_output, goal_output_tensor)
                        #pass

                    if retrain == 0:
                        #print (str(loss1) + '  ' + str(loss2))
                        loss = (loss1 + loss2) # update using all losses
                        #print(loss1, loss2, loss3)
                    elif retrain == 1:
                        loss = loss1 # update using just the ground loss
                    elif retrain == 2:
                        loss = loss2 # update using just the localization loss
                    else:
                        loss = loss3 # update using just the goal loss

                    # backward + optimize only if in training phase
                    if phase == 'train':
                        loss.backward()
                        opt.step()

                # statistics
                if retrain == 0:
                    running_loss += loss.item() * loc_inputs.size(0)
                elif retrain == 1:
                    running_loss += loss.item() * gnd_inputs.size(0)
                elif retrain == 2:
                    running_loss += loss.item() * loc_inputs.size(0)
                else:
                    running_loss += loss.item() * goal_inputs.size(0)

                pbar.update(1)
                sleep(0.01) #delay to print stats
            pbar.close()

            if phase == 'train': #adjust the learning rate if training
                scheduler.step()

            # With the cosine annealing scheduler, the learning rate is
            # gradually reduced.  If the learning rate is very small, then
            # reset the scheduler to do a warm restart
            if opt.param_groups[0]['lr'] < .0000001:
                tmax = int(tmax * tmax_factor)
                scheduler = lr_scheduler.CosineAnnealingLR(opt, T_max=tmax, verbose=True)
                print ("New T_max: " + str(tmax))

            #epoch_loss = running_loss / ground_dataset_sizes[phase]
            if retrain == 0:
                epoch_loss = running_loss / localization_dataset_sizes[phase]
            if retrain == 1:
                epoch_loss = running_loss / ground_dataset_sizes[phase]
            if retrain == 2:
                epoch_loss = running_loss / localization_dataset_sizes[phase]
            else:
                epoch_loss = running_loss / goal_dataset_sizes[phase]

            print('{} Loss: {:.4f}'.format(phase, epoch_loss))

            # deep copy the model
            if phase == 'val' and epoch_loss < best_loss:
                best_loss = epoch_loss
                best_model_wts = copy.deepcopy(model.state_dict())
                print ("Saving new best model...")
                model.eval()
                torch.save(model, 'inference.pt')
                torch.save({
                    'epoch': epoch,
                    'model_state_dict': model.state_dict()
                    #'optimizer_state_dict': optimizer.state_dict(),
                    #'loss': loss
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
        ground_w = 10
        loc_w = 1
        goal_w = 1
    elif len(sys.argv) > 1 and sys.argv[1] == str(2):
        retrain = 2 # retrain localization
        ground_w = 1
        loc_w = 10
        goal_w = 1
    elif len(sys.argv) > 1 and sys.argv[1] == str(3):
        retrain = 3 # retrain goal
        ground_w = 1
        loc_w = 1
        goal_w = 10
    else:
        retrain = 0 # initial training
        ground_w = 6
        loc_w = 5
        goal_w = 1

    # create the dataloader for the ground dataset
    ground_train_fp, ground_val_fp = ground_augment.ground_setup_dir()
    ground_train_d, ground_val_d = ground_augment.create_datasets(ground_train_fp, ground_val_fp)

    ground_image_datasets = {}
    ground_image_datasets['train'] = ground_train_d
    ground_image_datasets['val'] = ground_val_d

    ground_dataloaders = {x: torch.utils.data.DataLoader(ground_image_datasets[x], \
                   batch_size=32, shuffle=True, num_workers=ground_w) for x in ['train', 'val']}

    ground_dataset_sizes = {x: len(ground_image_datasets[x]) for x in ['train', 'val']}

    # create the dataloader for the localization dataset
    localization_train_fp, localization_val_fp = localization_augment.localization_setup_dir()
    localization_train_d, localization_val_d = localization_augment.create_datasets(localization_train_fp, localization_val_fp)

    localization_image_datasets = {}
    localization_image_datasets['train'] = localization_train_d
    localization_image_datasets['val'] = localization_val_d

    localization_dataloaders = {x: torch.utils.data.DataLoader(localization_image_datasets[x], \
                   batch_size=32, shuffle=True, num_workers=loc_w) for x in ['train', 'val']}

    localization_dataset_sizes = {x: len(localization_image_datasets[x]) for x in ['train', 'val']}

    # create the dataloader for the goal dataset
    goal_train_fp, goal_val_fp = goal_augment.goal_setup_dir()
    goal_train_d, goal_val_d = goal_augment.create_datasets(goal_train_fp, goal_val_fp)

    goal_image_datasets = {}
    goal_image_datasets['train'] = goal_train_d
    goal_image_datasets['val'] = goal_val_d

    goal_dataloaders = {x: torch.utils.data.DataLoader(goal_image_datasets[x], \
                   batch_size=32, shuffle=True, num_workers=goal_w) for x in ['train', 'val']}

    goal_dataset_sizes = {x: len(goal_image_datasets[x]) for x in ['train', 'val']}

    print (ground_dataset_sizes)
    print (len(ground_dataloaders['train']))
    print (localization_dataset_sizes)
    print (len(localization_dataloaders['val']))
    print (goal_dataset_sizes)
    print (len(goal_dataloaders['val']))

    # create the model
    # total outputs is 80 (ground detection) and 66 (localization)

    if retrain == 0: # initial training
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2, load=retrain)
        m.build()
        model = m

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        optimizer = optim.AdamW(model.parameters(), lr=0.005)

    elif retrain == 1: # retraining for ground detection
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2, load=retrain)
        m.build()
        model = m
        print ("-----------Retraining model for ground detection--------------")

        checkpoint = torch.load('test.pt')
        model.load_state_dict(checkpoint['model_state_dict'])

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        model = pretrained_model.Pretrained_Model.set_fixed_ground(model)

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.005)

    elif retrain == 2: # retraining for localization
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2, load=retrain)
        m.build()
        model = m
        print ("-----------Retraining model for localizaiton--------------")

        checkpoint = torch.load('test.pt')
        model.load_state_dict(checkpoint['model_state_dict'])

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        model = pretrained_model.Pretrained_Model.set_fixed_localization(model)

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.005)

    elif retrain == 3: # retraining for goal
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2, load=retrain)
        m.build()
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
    ground_criterion = nn.L1Loss()        # use L1 for ground detection
    #localization_criterion = nn.MSELoss() # use mean squared error for localization
    localization_criterion = nn.MSELoss() # use mean squared error for localization
    goal_criterion = nn.L1Loss()        # use L1 for goal prediction

    exp_lr_scheduler = lr_scheduler.CosineAnnealingLR(optimizer, T_max=10, verbose=True)

    # sys.exit()

    #train the model
    model = train_model(
        model, ground_criterion, localization_criterion, goal_criterion,
        optimizer, exp_lr_scheduler, num_epochs=203)

