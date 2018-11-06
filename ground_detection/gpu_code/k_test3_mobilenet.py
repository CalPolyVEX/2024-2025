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
image_list = []
full_path_image_list = []
height = 270
width = 480
#batch_size = 32
batch_size = 8
b_size = 0
split_size = 5
split_size = 3
epochs = 500

def init_program():
   global images, num_lidar_points, x_train, y_train, all_filenames, image_list
   global full_path_image_list, b_size, width, height

   #get image data
   image_list = os.listdir(str(width) + "_images")
   image_list.sort()
   b_size = int(len(image_list) / split_size)
   image_list = image_list[0:(b_size*split_size)]
   for f in image_list:
      if f.endswith(".jpg"):
         filename = os.path.join(str(width) + "_images/", f)
         full_path_image_list.append(filename)

   if 1 == 0:
      for f in image_list:
         if f.endswith(".jpg"):
            filename = os.path.join(str(width) + "_images/", f)
            full_path_image_list.append(filename)
            img = load_img(filename)

            x = img_to_array(img)
            x = x.astype('float32')
            #print x.shape
            mean = np.mean(x,axis=(0,1))
            #x = x - mean
            #print mean

            x_train.append(x)

   #get lidar data
   data_list = os.listdir(str(width) + "_data")
   data_list.sort()
   data_list = data_list[0:(b_size*split_size)]

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

   print ('shape', x_train.shape)
   print ('shape', y_train.shape)

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

#def IMDB_WIKI(X_samples, y_samples, batch_size=1000):
def IMDB_WIKI(X_samples, y_samples, batch_size=10000):
  #print X_samples
  X_samples = np.asarray(X_samples)
  #X_samples, y_samples = shuffle_in_unison(X_samples,y_samples)
  batch_size = len(X_samples) / batch_size
  X_batches = np.split(X_samples, batch_size)
  y_batches = np.split(y_samples, batch_size)
  
  for b in range(len(X_batches)):
    x = np.array(map(load_image, X_batches[b]))
    y = np.array(y_batches[b])
    yield x, y

def t():
   global callbacks, checkpoint, b_size

   n_epoch = 500
   for e in range(n_epoch):
      print "Epoch", e
      for X_train, y_tr in IMDB_WIKI(full_path_image_list, y_train, b_size): # chunks of 100 images
         model.fit(X_train, y_tr, batch_size=16, epochs=1, verbose=1, validation_split=.5, shuffle=False, callbacks=callbacks)
         #model.train_on_batch(X_train,y_tr)
   sys.exit()


model = baseline_model()
model.summary()

t()
