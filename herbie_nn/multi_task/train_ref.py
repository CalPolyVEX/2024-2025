import copy
import os, time, sys, random
import ground_augment
import localization_augment
import goal_augment
import turn_augment
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
from torchinfo import summary

cudnn.benchmark = True

device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

def train_model(model, criterion, optimizer, scheduler, num_epochs=25):
    since = time.time() # get the starting time of training

    best_loss = 100000.0
    tmax_factor = 1.5 # with warm restart, multiply the max factor by this amount

    if retrain == 0 or retrain == 1:
        tmax = 20 # after tmax iterations, the learning rate is reset
    else:
        tmax = 10 # after tmax iterations, the learning rate is reset

    for epoch in range(num_epochs):
        print('Epoch {}/{}'.format(epoch, num_epochs - 1))
        print('-' * 10)

        # Each epoch has a training and validation phase
        for phase in ['train', 'val']:
            if phase == 'train':
                model.train()  # Set model to training mode

                # for states 2,3,4 freeze the backbone
                if (retrain == 2) or (retrain ==3) or (retrain == 4):
                    model.m.eval()

                    #freeze the ground backbone
                    model.m1.eval()
                    model.ground_out.eval()
                elif retrain == 1: # training the ground boundary
                    model.m.eval()
                    model = pretrained_model.Pretrained_Model.set_train_ground(model)
                elif retrain == 0: # freeze the ground boundary backbone
                    model.m1.eval()
                    model.ground_out.eval()

                    if epoch < 5: #in the first few epochs, freeze the pretrained layers
                        model.m.eval()

                if retrain == 2:
                    model = pretrained_model.Pretrained_Model.set_train_loc(model)
                elif retrain == 3:
                    model = pretrained_model.Pretrained_Model.set_train_goal(model)
                elif retrain == 4:
                    model = pretrained_model.Pretrained_Model.set_train_turn(model)

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

            turn_iterations = len(turn_dataloaders[phase])
            turn_iterator = iter(turn_dataloaders[phase])

            if retrain == 0:
                iterations = int(1.5 * loc_iterations) # account for the other training iterations
            elif retrain == 1:
                iterations = ground_iterations
            elif retrain == 2:
                iterations = loc_iterations
            elif retrain == 3:
                iterations = goal_iterations
            elif retrain == 4:
                iterations = turn_iterations

            # Iterate over data.
            pbar = tqdm(total=iterations,desc=phase,ncols=70)

            total_loss = [0] * 4  # accumulate the losses
            total_iter = [0] * 4  # count the iterations of each

            train_counter = 0

            # generate training schedule
            training_schedule_length = 20
            # training_schedule = [1,2,1,2,3,1,2,1,2,3, \
            #                      1,2,1,2,3,1,2,1,2,3]
            training_schedule = [1,2,3,1,2,3,1,2,3,1, \
                                 2,3,1,2,3,1,2,3,1,2]

            assert len(training_schedule) == training_schedule_length

            for i in range(iterations):
                if i % training_schedule_length == 0:
                    train_counter = 0

                loc_train = 0
                turn_train = 0
                goal_train = 0

                if training_schedule[train_counter] == 1:
                    loc_train = 1
                elif training_schedule[train_counter] == 2:
                    turn_train = 1
                else:
                    goal_train = 1

                train_counter += 1

                ground_flag = 0
                loc_flag = 0
                turn_flag = 0
                goal_flag = 0

                if retrain == 0:
                    if loc_train == 1: # localization batch
                        inputs, labels, turn_labels = next(loc_iterator)

                        inputs = inputs.to(device)
                        output_tensor = labels.to(device)
                        loc_flag = 1

                        # if phase == 'train':
                        #     model = pretrained_model.Pretrained_Model.set_train_loc(model)
                    elif turn_train == 1: # turn batch
                        try:
                            inputs, labels = next(turn_iterator)
                        except:
                            turn_iterator = iter(turn_dataloaders[phase])
                            inputs, labels = next(turn_iterator)
                            print('---error---')

                        inputs = inputs.to(device)
                        output_tensor = labels.to(device)
                        turn_flag = 1

                        # if phase == 'train':
                        #     model = pretrained_model.Pretrained_Model.set_train_turn(model)
                    elif goal_train == 1: # goal batch
                        try:
                            inputs, labels = next(goal_iterator)
                        except:
                            goal_iterator = iter(goal_dataloaders[phase])
                            inputs, labels = next(goal_iterator)

                        inputs = inputs.to(device)
                        output_tensor = labels.to(device)
                        goal_flag = 1

                        # if phase == 'train':
                        #     model = pretrained_model.Pretrained_Model.set_train_goal(model)
                elif retrain == 1:
                    inputs, labels = next(ground_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)
                    ground_flag = 1
                elif retrain == 2:
                    inputs, labels, turn_labels = next(loc_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)
                    loc_flag = 1
                elif retrain == 3:
                    inputs, labels = next(goal_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)
                    goal_flag = 1
                elif retrain == 4:
                    inputs, labels = next(turn_iterator)

                    inputs = inputs.to(device)
                    output_tensor = labels.to(device)
                    turn_flag = 1

                # zero the parameter gradients
                optimizer.zero_grad()

                # Run the forward pass and track history if only in training
                with torch.set_grad_enabled(phase == 'train'):
                    outputs = model(inputs)
                    preds = outputs

                    if retrain == 0:
                        if loc_train == 1: #localization
                             # loss = 3.5 * criterion[1](outputs[1], output_tensor)
                             loss = criterion[1](outputs[1], output_tensor)
                             total_loss[1] += loss.item() * inputs.size(0)
                             total_iter[1] += 1
                             assert loc_flag == 1
                        elif turn_train == 1: #turn
                             #loss = .3 * criterion[2](outputs[2], output_tensor)
                             loss = criterion[2](outputs[2], output_tensor)
                             total_loss[2] += loss.item() * inputs.size(0)
                             total_iter[2] += 1
                             assert turn_flag == 1
                        elif goal_train == 1: #goal
                             loss = criterion[3](outputs[3], output_tensor)
                             total_loss[3] += loss.item() * inputs.size(0)
                             total_iter[3] += 1
                             assert goal_flag == 1
                    elif retrain == 1:
                        loss = criterion[0](outputs[0], output_tensor)
                        total_loss[0] += loss.item() * inputs.size(0)
                        total_iter[0] += 1
                        assert ground_flag == 1
                    elif retrain == 2: # retrain localization
                        loss = criterion[1](outputs[1], output_tensor)
                        total_loss[1] += loss.item() * inputs.size(0)
                        total_iter[1] += 1
                        assert loc_flag == 1
                    elif retrain == 3: # retrain goal
                        loss = criterion[3](outputs[3], output_tensor)
                        total_loss[3] += loss.item() * inputs.size(0)
                        total_iter[3] += 1
                        assert goal_flag == 1
                    elif retrain == 4: # retrain turn
                        loss = criterion[2](outputs[2], output_tensor)
                        total_loss[2] += loss.item() * inputs.size(0)
                        total_iter[2] += 1
                        assert turn_flag == 1

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

            if retrain == 0:
                epoch_loss = running_loss / (1.5*loc_dataset_sizes[phase])

                # compute the total loss for each output head
                for i in range(len(total_loss)):
                    total_loss[i] /= float(1.5*loc_dataset_sizes[phase])
            elif retrain == 1:
                epoch_loss = running_loss / ground_dataset_sizes[phase]

                # compute the total loss for each output head
                for i in range(len(total_loss)):
                    total_loss[i] /= float(ground_dataset_sizes[phase])
            elif retrain == 2:
                epoch_loss = running_loss / loc_dataset_sizes[phase]

                # compute the total loss for each output head
                for i in range(len(total_loss)):
                    total_loss[i] /= float(loc_dataset_sizes[phase])
            elif retrain == 3:
                epoch_loss = running_loss / goal_dataset_sizes[phase]

                # compute the total loss for each output head
                for i in range(len(total_loss)):
                    total_loss[i] /= float(goal_dataset_sizes[phase])
            elif retrain == 4:
                epoch_loss = running_loss / turn_dataset_sizes[phase]

                # compute the total loss for each output head
                for i in range(len(total_loss)):
                    total_loss[i] /= float(turn_dataset_sizes[phase])

            print('{} Loss: {:.4f}'.format(phase, epoch_loss), end = '')


            print('  ({:.4f}, {:.4f}, {:.4f}, {:.4f})'.format(total_loss[0], \
                total_loss[1], total_loss[2], total_loss[3]))
            print('iterations:  ({:.4f}, {:.4f}, {:.4f}, {:.4f})'.format(total_iter[0], \
                total_iter[1], total_iter[2], total_iter[3]))

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
    # if there is no argument, then the state is 0 (train the 3 output heads)
    # if the argument is 1, then train the ground branch
    if len(sys.argv) > 1 and sys.argv[1] == str(1):
        retrain = 1 # train ground
        g_workers = 12
        l_workers = 0
        turn_workers = 0
        go_workers = 0
    elif len(sys.argv) > 1 and sys.argv[1] == str(2):
        retrain = 2 # retrain localization
        g_workers = 0
        l_workers = 12
        turn_workers = 0
        go_workers = 0
    elif len(sys.argv) > 1 and sys.argv[1] == str(3):
        retrain = 3 # retrain goal
        g_workers = 0
        l_workers = 0
        turn_workers = 0
        go_workers = 12
    elif len(sys.argv) > 1 and sys.argv[1] == str(4):
        retrain = 4 # retrain turn
        g_workers = 0
        l_workers = 0
        turn_workers = 12
        go_workers = 0
    else:
        retrain = 0 # initial training
        g_workers = 0
        l_workers = 4
        go_workers = 4
        turn_workers = 4

    # create the dataloader for the ground dataset
    ground_train_fp, ground_val_fp = ground_augment.ground_setup_dir()
    ground_train_d, ground_val_d = ground_augment.create_datasets(ground_train_fp, ground_val_fp)

    ground_image_datasets = {}
    ground_image_datasets['train'] = ground_train_d
    ground_image_datasets['val'] = ground_val_d

    ground_dataloaders = {x: torch.utils.data.DataLoader(ground_image_datasets[x], \
                   batch_size=32, shuffle=True, num_workers=g_workers) for x in ['train', 'val']}

    ground_dataset_sizes = {x: len(ground_image_datasets[x]) for x in ['train', 'val']}

    # localization dataloader
    loc_train_fp, loc_val_fp = localization_augment.localization_setup_dir()
    loc_train_d, loc_val_d = localization_augment.create_datasets(loc_train_fp, loc_val_fp)

    loc_image_datasets = {}
    loc_image_datasets['train'] = loc_train_d
    loc_image_datasets['val'] = loc_val_d

    loc_dataloaders = {x: torch.utils.data.DataLoader(loc_image_datasets[x], \
                   batch_size=32, shuffle=True, num_workers=l_workers) for x in ['train', 'val']}

    loc_dataset_sizes = {x: len(loc_image_datasets[x]) for x in ['train', 'val']}

    # goal dataloader
    goal_train_fp, goal_val_fp = goal_augment.goal_setup_dir()
    goal_train_d, goal_val_d = goal_augment.create_datasets(goal_train_fp, goal_val_fp)

    goal_image_datasets = {}
    goal_image_datasets['train'] = goal_train_d
    goal_image_datasets['val'] = goal_val_d

    goal_dataloaders = {x: torch.utils.data.DataLoader(goal_image_datasets[x], \
        batch_size=32, shuffle=True, num_workers=go_workers) for x in ['train', 'val']}

    goal_dataset_sizes = {x: len(goal_image_datasets[x]) for x in ['train', 'val']}

    # turn dataloader
    turn_train_fp, turn_val_fp = turn_augment.turn_setup_dir()
    turn_train_d, turn_val_d = turn_augment.create_datasets(turn_train_fp, turn_val_fp)

    turn_image_datasets = {}
    turn_image_datasets['train'] = turn_train_d
    turn_image_datasets['val'] = turn_val_d

    turn_dataloaders = {x: torch.utils.data.DataLoader(turn_image_datasets[x], \
        batch_size=32, shuffle=True, num_workers=turn_workers) for x in ['train', 'val']}

    turn_dataset_sizes = {x: len(turn_image_datasets[x]) for x in ['train', 'val']}

    #dataset_sizes = {x: len(image_datasets[x]) for x in ['train', 'val']}
    print ("ground dataset: ", end = '')
    print (ground_dataset_sizes)
    print ("localization dataset: ", end = '')
    print (loc_dataset_sizes)
    print ("goal dataset: ", end = '')
    print (goal_dataset_sizes)
    print ("turn dataset: ", end = '')
    print (turn_dataset_sizes)

    #create the model
    if retrain == 0:
        m = pretrained_model.Pretrained_Model(shape=(360,640,3), num_outputs1=80,
            num_outputs2=64, goal_outputs=2)
        model = m
        #summary(model, input_size=(1, 3, 360, 640))

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        #optimizer = optim.AdamW(model.parameters(), lr=0.0005)
        optimizer = optim.AdamW(model.parameters(), lr=0.0003)

    elif retrain == 1:
        # retrain the ground detection
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2)
        model = m
        print ("-----------Training model for ground detection--------------")

        checkpoint = torch.load('test.pt')
        model.load_state_dict(checkpoint['model_state_dict'])

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        #summary(model, input_size=(1, 3, 360, 640))

        model = pretrained_model.Pretrained_Model.set_train_ground(model)
        model.m.eval() # set the backbone of the other output heads to eval

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.0007)

    elif retrain == 2:
        # retrain the localization head
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2)
        model = m
        print ("-----------Retraining model for localization--------------")

        checkpoint = torch.load('test.pt')
        model.load_state_dict(checkpoint['model_state_dict'])

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        model = pretrained_model.Pretrained_Model.set_fixed_localization(model)
        model = pretrained_model.Pretrained_Model.set_train_loc(model)

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.002)

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
        model = pretrained_model.Pretrained_Model.set_train_goal(model)

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.003)

    elif retrain == 4:
        # retrain the turn head
        m = pretrained_model.Pretrained_Model(
            shape=(360,640,3), num_outputs1=80, num_outputs2=64, goal_outputs=2)
        model = m
        print ("-----------Retraining model for turn--------------")

        checkpoint = torch.load('test.pt')
        model.load_state_dict(checkpoint['model_state_dict'])

        if torch.cuda.is_available(): #send the model to the GPU if available
           model.cuda()

        model = pretrained_model.Pretrained_Model.set_fixed_turn(model)
        model = pretrained_model.Pretrained_Model.set_train_turn(model)

        optimizer = optim.AdamW(filter(lambda p: p.requires_grad, \
            model.parameters()), lr=0.0003)

    #configure the losses: ground boundary, localization, turn classification, goal
    #criterion = [nn.L1Loss(), nn.MSELoss(), nn.L1Loss(), nn.L1Loss()]
    if retrain == 4:
        criterion = [nn.L1Loss(), nn.MSELoss(), nn.MSELoss(), nn.L1Loss()]
    else:
        criterion = [nn.L1Loss(), nn.MSELoss(), nn.MSELoss(), nn.L1Loss()]

    # optimizer = optim.AdamW(model.parameters(), lr=0.005)
    if retrain == 0 or retrain == 1:
        exp_lr_scheduler = lr_scheduler.CosineAnnealingLR(optimizer, T_max=20, verbose=True)
    else:
        exp_lr_scheduler = lr_scheduler.CosineAnnealingLR(optimizer, T_max=10, verbose=True)

    #train the model
    #model = train_model(model, criterion, optimizer, exp_lr_scheduler, num_epochs=480)
    model = train_model(model, criterion, optimizer, exp_lr_scheduler, num_epochs=203)

