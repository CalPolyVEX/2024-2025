#!/bin/bash

#SBATCH -N 1
#SBATCH -o output3
#SBATCH -e error3
#SBATCH -p defq
#SBATCH --gres=gpu:1
module load shared cm-ml-python3deps/2.0.0 ml-python3/tensorflow/1.10.1  openmpi3/gcc/64/3.0.0 keras
srun --gres=gpu:1 /home/jseng/test/ue4/ground_detection/gpu_code/bishop/k_test3_mobilenet.py
