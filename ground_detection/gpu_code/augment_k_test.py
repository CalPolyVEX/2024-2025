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
import Augmentor

import numpy as np
import os, random, sys

l2_lambda = .0001
num_lidar_points = 0
images = {}
all_filenames = []
x_train = []
y_train = []
img_rows = 240
img_cols = 320
batch_size = 32
#batch_size = 16
epochs = 400

def init_program():
   global images, num_lidar_points, x_train, y_train, all_filenames

   #get image data
   image_list = os.listdir("320_images")
   image_list.sort()
   for f in image_list:
      if f.endswith(".jpg"):
         filename = os.path.join("320_images/", f)
         img = load_img(filename)
         #t = np.mean(img, axis=2)
         #for y in np.nditer(t, op_flags=['readwrite']):
         #   print y
#test convert to grayscale
         #x = img_to_array(t)

         x = img_to_array(img)
         #print x

         #print t.shape
         images[f] = x
         x_train.append(images[f])

   #get lidar data
   data_list = os.listdir("320_data")
   data_list.sort()

   #check that all the training images and data files match
   for i in range(len(image_list)):
      image = image_list[i]
      datafile = data_list[i]

      image = image.replace(".jpg","")
      image = image.replace("320_","")
      datafile = datafile.replace(".txt","")
      datafile = datafile.replace("320_gt_","")

      #print str(image), str(datafile)

      if str(image) != str(datafile):
         print "mismatch" + str(datafile)
         sys.exit(0)

   for f in data_list:
      if f.endswith(".txt"):
         filename = os.path.join("320_data/", f)
            
         #read the data file into a list
         with open(filename) as f:
            lines = f.readlines()
         lines = [float((x.split(',')[1]).strip()) for x in lines] #run strip for each line
         num_lidar_points = len(lines)
         #print lines

         lidar_data = np.array(lines, dtype=np.float32)
         #print lidar_data.shape

         assert len(lidar_data) == 32, filename
         y_train.append(lidar_data)

   x_train = np.array(x_train)
   x_train = x_train.astype('float32')
   x_train /= 255

   y_train = np.array(y_train)
   y_train = y_train.astype('float32')
   y_train /= img_rows

   print ('shape', x_train.shape)
   print ('shape', y_train.shape)

init_program()
print "Initialization complete"
#quit()

def baseline_model():
   rmsprop = optimizers.RMSprop(lr=0.1)
   sgd = optimizers.SGD(lr=0.01)
   model = Sequential()
   model.add(Conv2D(96, kernel_size=(5,5), strides=2, activation='relu', input_shape=(240, 320, 3)))
   #model.add(Conv2D(96, kernel_size=(5,5), strides=2, activation='relu', input_shape=(240, 320, 3), kernel_regularizer=l2(l2_lambda)))
   model.add(BatchNormalization())
   model.add(MaxPooling2D(pool_size=(2, 2)))
   model.add(Dropout(0.35))
   #model.add(Conv2D(128, (5, 5), activation='relu', kernel_regularizer=l2(l2_lambda)))
   model.add(Conv2D(96, (5, 5), activation='relu'))
   model.add(BatchNormalization())
   model.add(MaxPooling2D(pool_size=(2, 2)))
   model.add(Dropout(0.35))
   #model.add(Conv2D(128, (3, 3), activation='relu', kernel_regularizer=l2(l2_lambda)))
   model.add(Conv2D(96, (3, 3), activation='relu'))
   model.add(BatchNormalization())
   model.add(MaxPooling2D(pool_size=(2, 2)))
   model.add(Dropout(0.35))
   #model.add(Conv2D(128, (3, 3), activation='relu', kernel_regularizer=l2(l2_lambda)))
   model.add(Conv2D(96, (3, 3), activation='relu'))
   model.add(BatchNormalization())
   model.add(MaxPooling2D(pool_size=(2, 2)))
   model.add(Dense(96, activation='relu'))
   model.add(BatchNormalization())
   model.add(Dropout(0.25))
   model.add(Dense(96, activation='relu'))
   model.add(BatchNormalization())
   model.add(Dropout(0.25))
   model.add(Dense(96, activation='relu'))
   #model.add(Dropout(0.25))
   model.add(Flatten())
   model.add(Dense(num_lidar_points))
   #model.compile(loss=keras.losses.categorical_crossentropy, 
   model.compile(loss=keras.losses.mean_absolute_error,
                 #optimizer=sgd, 
                 #optimizer=keras.optimizers.Adadelta(), 
                 optimizer=keras.optimizers.Adam(), 
                 metrics=['accuracy'])
   return model

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

model = baseline_model()
print model.summary()
p = []

def augmentation():
   global p
   print 'test'
   p = Augmentor.Pipeline("./input")
   # Point to a directory containing ground truth data.
   # Images with the same file names will be added as ground truth data
   # and augmented in parallel to the original data.
   p.ground_truth("./ground_output")
   # Add operations to the pipeline as normal:
   p.rotate(probability=1, max_left_rotation=5, max_right_rotation=5)
   p.flip_left_right(probability=0.5)
   p.zoom_random(probability=0.5, percentage_area=0.8)
   p.flip_top_bottom(probability=0.5)
   p.resize(probability=1.0, width=320, height=240)

   g = p.keras_generator(batch_size=1)
   images, labels = next(g)
   print images.shape
   print labels.shape
   #p.sample(50)

augmentation()
sys.exit(0)

for x in range(epochs):
   print 'Epoch' + str(x)
   model.fit(x_train, y_train, batch_size=batch_size, epochs=1, verbose=1, validation_split=.015, shuffle=True, callbacks=callbacks)
   model.save('my_model.h5')