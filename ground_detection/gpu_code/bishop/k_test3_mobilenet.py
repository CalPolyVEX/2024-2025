#!/usr/bin/python3.6
from keras.preprocessing.image import load_img, img_to_array
from keras import backend as K
from keras.models import Sequential
from keras.layers import Dense, Dropout, Flatten
from keras.layers import Conv2D, MaxPooling2D
from keras.wrappers.scikit_learn import KerasRegressor
from keras import optimizers
from keras.regularizers import l2
from keras.layers.normalization import BatchNormalization
from keras.callbacks import ModelCheckpoint
import keras

import numpy as np
import os, random, sys
import mobilenet_v2

l2_lambda = .0001
num_lidar_points = 0
images = {}
all_filenames = []
x_train = []
y_train = []
x_val = []
y_val = []
image_list = []
full_path_image_list = []
height = 270
width = 480
#batch_size = 32
batch_size = 8
b_size = 5000
split_size = 25
epochs = 500

def init_program():
   global images, num_lidar_points, x_train, y_train, all_filenames, image_list
   global full_path_image_list, b_size, width, height, x_val, y_val

   #get image data
   image_list = os.listdir(str(width) + "_images")
   image_list.sort()
   #b_size = int(len(image_list) / split_size)
   #image_list = image_list[0:(b_size*split_size)]

   for f in image_list:
      if f.endswith(".jpg"):
         filename = os.path.join(str(width) + "_images/", f)
         full_path_image_list.append(filename)

   #get lidar data
   data_list = os.listdir(str(width) + "_data")
   data_list.sort()
   #data_list = data_list[0:(b_size*split_size)]

   #check that all the training images and data files match
   for i in range(len(image_list)):
      image = image_list[i]
      datafile = data_list[i]

      image = image.replace(".jpg","")
      datafile = datafile.replace(".txt","")

      #print str(image), str(datafile)

      if str(image) != str(datafile):
         print ("mismatch" + str(datafile))
         sys.exit(0)

   for f in data_list:
      if f.endswith(".txt"):
         filename = os.path.join(str(width) + "_data/", f)
            
         #read the data file into a list
         with open(filename) as f:
            lines = f.readlines()
         lines = [float((x.split(',')[1]).strip()) for x in lines] #run strip for each line
         num_lidar_points = len(lines)
         #print lines

         lidar_data = np.array(lines, dtype=np.float32)
         #print lidar_data.shape

         assert len(lidar_data) == 48, filename
         y_train.append(lidar_data)

   x_train = np.array(x_train)
   x_train = x_train.astype('float32')
   x_train /= 255

   c = list(zip(full_path_image_list, y_train))
   random.shuffle(c)
   full_path_image_list, y_train = zip(*c)

   y_train = np.array(y_train)
   y_train = y_train.astype('float32')
   y_train /= int(height)

   training_set_size = 25000
   val_size = len(y_train) - training_set_size
   x_val = full_path_image_list[-val_size:]
   y_val = y_train[-val_size:]

   full_path_image_list = full_path_image_list[0:training_set_size]
   y_train = y_train[0:training_set_size]

   print ('full_path_image_list shape:' , len(full_path_image_list))
   print ('y_val shape', y_val.shape)
   print ('y_train shape ', y_train.shape)

init_program()
print ("Initialization complete")
#quit()

def baseline_model():
   global width, height

   rmsprop = optimizers.RMSprop(lr=0.1)
   sgd = optimizers.SGD(lr=0.01)

   model = mobilenet_v2.MobileNetv2((int(height), int(width), 3), 48)
   model.compile(loss=keras.losses.mean_absolute_error,
                 #optimizer=sgd, 
                 #optimizer=keras.optimizers.Adadelta(), 
                 optimizer=keras.optimizers.Adam(), 
                 metrics=['accuracy'])
   return model

def load_image(img_path):
  im = load_img(img_path)
  im = img_to_array(im)
  im = im.astype('float32')
  im /= 255
  return im #converts image to numpy array

# Prepare model model saving directory.
save_dir = os.path.join(os.getcwd(), 'saved_models')
model_name = sys.argv[0] + '_model.h5' 
if not os.path.isdir(save_dir):
    os.makedirs(save_dir)
filepath = os.path.join(save_dir, model_name)

# Prepare callbacks for model saving and for learning rate adjustment.
checkpoint = ModelCheckpoint(filepath=filepath,
                             monitor='val_loss',
                             verbose=1,
                             save_best_only=True)

callbacks = [checkpoint]

def shuffle_in_unison(a, b):
    assert len(a) == len(b)
    shuffled_a = np.empty(a.shape, dtype=a.dtype)
    shuffled_b = np.empty(b.shape, dtype=b.dtype)
    permutation = np.random.permutation(len(a))
    for old_index, new_index in enumerate(permutation):
        shuffled_a[new_index] = a[old_index]
        shuffled_b[new_index] = b[old_index]
    return shuffled_a, shuffled_b

def IMDB_WIKI(X_samples, y_samples, batch_size=10000):
  #print X_samples
  X_samples = np.asarray(X_samples)
  #X_samples, y_samples = shuffle_in_unison(X_samples,y_samples)
  batch_size = len(X_samples) / batch_size
  X_batches = np.split(X_samples, batch_size)
  y_batches = np.split(y_samples, batch_size)
  
  for b in range(len(X_batches)):
    x = np.array(list(map(load_image, X_batches[b])))
    y = np.array(y_batches[b])
    yield x, y

def build_validation_set(X_samples, y_samples):
   X_samples = np.asarray(X_samples)
   #print ("X_samples:" , X_samples)
   
   x_v = np.array(list(map(load_image, X_samples)))
   y_v = np.array(y_samples)
   #print ("x_v" , x_v.shape)
   #print ("x_v" , x_v)
   return x_v, y_v

def fit():
   global callbacks, checkpoint, b_size, x_val, y_val, y_train, full_path_image_list

   #read in validation set
   x_val, y_val = build_validation_set(x_val, y_val)

   n_epoch = 300
   for e in range(n_epoch):
      print ("Epoch", e)
      for X_train, y_tr in IMDB_WIKI(full_path_image_list, y_train, b_size): # chunks of 100 images
         #model.fit(X_train, y_tr, batch_size=16, epochs=1, verbose=1, validation_split=.6, shuffle=False, callbacks=callbacks)
         #print ("X_train shape:", type(X_train))
         #print("X_train shape:", X_train.shape)
         #print("y_train shape:", y_tr.shape)
         #print("x_val shape:", x_val.shape)
         #print("y_val shape:", y_val.shape)
         #print("type x_train: ", full_path_image_list)
         model.fit(X_train, y_tr, batch_size=32, epochs=1, verbose=1, shuffle=True, callbacks=callbacks, validation_data=(x_val, y_val))
   sys.exit()

model = baseline_model()
model.summary()

fit()
