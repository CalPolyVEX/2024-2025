from keras.preprocessing.image import load_img, img_to_array
from keras import backend as K
from keras.models import Sequential, load_model
from keras.layers import Dense, Dropout, Flatten
from keras.layers import Conv2D, MaxPooling2D
from keras.wrappers.scikit_learn import KerasRegressor
from keras import optimizers 
import keras

import numpy as np
import os, random
import sys, time

#if len(sys.argv) < 2:
#   print "need directory"
#   sys.exit(0)

images = {}
img_rows = 240
img_cols = 320
model = ''
file_data = []

def baseline_model():
   model = load_model('saved_models/k_test2.py_model.h5')

   #remove dropout layers
#   for k in model.layers:
#      if type(k) is keras.layers.Dropout:
#         model.layers.remove(k)
#      if type(k) is keras.layers.BatchNormalization:
#         model.layers.remove(k)
   print model.summary()

   return model

def init_program():
   global images, x_train, y_train, model, file_data

   for file in os.listdir("test_files"):
      if file.endswith(".jpg"):
         filename = os.path.join("test_files/", file)
         img = load_img(filename)
         x = img_to_array(img)
         #print x.shape
         images[file] = x
         #print images[file].shape

def run_inference_test():
   global file_data, images
   in_dir = './test_images/'

   time_counter = 0
   total_time = 0.0
   while 1 == 1:
      for filename in os.listdir(in_dir):
         if '.jpg' in filename:
            img = load_img(in_dir + '/' + filename)
            img_input = np.expand_dims(img, axis=0)
            img_input = img_input.astype('float32')
            img_input /= 255

            time1 = time.time()
            prediction = model.predict(img_input, 1, 1)
            time2 = time.time()

            if (time2 - time1) < .1:
               total_time += time2 - time1 
               time_counter += 1

               #print (time2-time1)
               print 'function took %0.3f ms' % ((time2-time1)*1000.0)
               print 'average function time %0.3f ms' % (total_time * 1000.0 / time_counter)
               prediction = [int(x*240) for x in prediction[0]]
               #print prediction

            counter = 5
            if 1 == 0:
               f = open(in_dir+'/'+filename.replace('jpg','txt'),'w')
               for x in prediction:
                  f.write(str(counter)+','+str(x)+'\n')
                  counter += 10
               f.close()

            #print prediction

            #prediction.tofile('foo3.csv',sep=',',format='%10.5f')

random.seed()
model = baseline_model()
#init_program()
#run_inference()
run_inference_test()
