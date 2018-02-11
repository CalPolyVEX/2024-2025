from keras.preprocessing.image import load_img, img_to_array
from keras import backend as K
from keras.models import Sequential
from keras.layers import Dense, Dropout, Flatten
from keras.layers import Conv2D, MaxPooling2D
from keras.wrappers.scikit_learn import KerasRegressor
from keras import optimizers
from keras.regularizers import l2
import keras

import numpy as np
import os, random

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
epochs = 150

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
   model.add(Conv2D(64, kernel_size=(5,5), strides=2, activation='relu', input_shape=(240, 320, 3), kernel_regularizer=l2(l2_lambda)))
   #model.add(Conv2D(64, kernel_size=(5,5), strides=2, activation='relu', input_shape=(240, 320, 1), kernel_regularizer=l2(l2_lambda)))
   model.add(MaxPooling2D(pool_size=(2, 2)))
   model.add(Dropout(0.25))
   model.add(Conv2D(128, (5, 5), activation='relu', kernel_regularizer=l2(l2_lambda)))
   model.add(MaxPooling2D(pool_size=(2, 2)))
   model.add(Dropout(0.25))
   model.add(Conv2D(64, (3, 3), activation='relu', kernel_regularizer=l2(l2_lambda)))
   model.add(MaxPooling2D(pool_size=(2, 2)))
   #model.add(Dropout(0.25))
   #model.add(Conv2D(192, (3, 3), activation='relu', kernel_regularizer=l2(l2_lambda)))
   model.add(Dropout(0.25))
   model.add(Conv2D(128, (3, 3), activation='relu', kernel_regularizer=l2(l2_lambda)))
   model.add(MaxPooling2D(pool_size=(2, 2)))
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

model = baseline_model()
print model.summary()

for x in range(epochs):
   print 'Epoch' + str(x)
   model.fit(x_train, y_train, batch_size=batch_size, epochs=1, verbose=1, validation_split=.01)
   model.save('my_model.h5')
