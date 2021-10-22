#!/usr/bin/bash
echo "Training complete network"
source ~/mpac_venv/bin/activate
cd /home/jseng/ue4/herbie_nn/multi_task
python train_ref.py > output0_$HOSTNAME    #train shared backbone
#python train_ref.py 2 > output1_$HOSTNAME  #train localization
python train_ref.py 3 > output2_$HOSTNAME  #train goal
#python train_ref.py 4 > output3_$HOSTNAME  #train turn
python train_ref.py 1 > output4_$HOSTNAME  #train ground detection
