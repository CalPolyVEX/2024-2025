import matplotlib 
matplotlib.use('Agg') 

import h5py, os, sys
import caffe
import numpy as np

#image_dir = '/home/nvidia/cnn_test/320_images'
data_dir = '/home/nvidia/cnn_test/320_data'

data_files = os.listdir(data_dir)

data_files.sort()

print data_files

#f = h5py.File('train.h5', 'w')
# Data's labels, each is a 4-dim vector
#f.create_dataset('label', (len(data_files), 32), dtype='f4')

# Fill in something with fixed pattern
# Regularize values to between 0 and 1, or SigmoidCrossEntropyLoss will not work

img_data = []

for i in range(len(data_files)):
   print data_files[i]

   with open (data_dir + '/' + data_files[i]) as H:
      lines = H.readlines()

   lines = [x.strip() for x in lines]
   lines = [int(x.split(',')[1]) for x in lines]
   print lines

   img_data.append(lines)

#f.close()
with h5py.File('train.h5','w') as H:
   H.create_dataset( 'X', data=img_data ) # note the name X given to the dataset!
   #H.create_dataset( 'y', data=y ) # note the name y given to the dataset!
