# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

from __future__ import absolute_import
from __future__ import division
#from __future__ import print_function

import argparse
import sys
import time, os, gc

import numpy as np
import tensorflow as tf

def read_tensor_from_image_file(file_name, input_height=299, input_width=299):
   input_name = "file_reader"
   output_name = "normalized"
   file_reader = tf.read_file(file_name, input_name)
   image_reader = tf.image.decode_jpeg(file_reader, channels = 3,
                                       name='jpeg_reader')
   float_caster = tf.cast(image_reader, tf.float32)
   float_caster /= 255.0
   dims_expander = tf.expand_dims(float_caster, 0);
   resized = tf.image.resize_bilinear(dims_expander, [input_height, input_width])
   #print type(resized)
   mean = tf.reduce_mean(resized,axis=(1,2))
   #print resized.shape
   #print mean
   normalized = tf.subtract(resized, mean)
   print normalized.get_shape()
   sess = tf.Session()
   result = sess.run(resized)

   return result

class GroundDetector:
   def __init__(self, protobuf_model, input_layer, output_layer):
      self.width = 480
      self.height = 270
      self.protobuf_model = protobuf_model
      self.input_layer = input_layer
      self.output_layer = output_layer
      self.input_name = "import/" + input_layer
      self.output_name = "import/" + output_layer

      #load the graph
      self.load_graph()

      self.input_operation = self.graph.get_operation_by_name(self.input_name);
      self.output_operation = self.graph.get_operation_by_name(self.output_name);
   
      self.sess = tf.Session(graph=self.graph)

   def load_graph(self):
      self.graph = tf.Graph()
      self.graph_def = tf.GraphDef()

      with open(self.protobuf_model, "rb") as f:
         self.graph_def.ParseFromString(f.read())
      with self.graph.as_default():
         tf.import_graph_def(self.graph_def)

   def run(self, input_image):
      results = self.sess.run(self.output_operation.outputs[0], {self.input_operation.outputs[0]: input_image})
      return results

if __name__ == "__main__":
   parser = argparse.ArgumentParser()
   parser.add_argument("--image", help="image to be processed")
   parser.add_argument("--graph", help="graph/model to be executed")
   parser.add_argument("--labels", help="name of file containing labels")
   parser.add_argument("--input_height", type=int, help="input height")
   parser.add_argument("--input_width", type=int, help="input width")
   parser.add_argument("--input_layer", help="name of input layer")
   parser.add_argument("--output_layer", help="name of output layer")
   args = parser.parse_args()

   if args.graph:
      model_file = args.graph
   if args.image:
      file_name = args.image
   if args.input_height:
      input_height = args.input_height
   if args.input_width:
      input_width = args.input_width
   if args.input_layer:
      input_layer = args.input_layer
   if args.output_layer:
      output_layer = args.output_layer

   testgd = GroundDetector('output_graph.pb', input_layer, output_layer)
   testgd1 = GroundDetector('output_graph.pb', input_layer, output_layer)

   #get jpg files
   dir1='../gpu_code/test_images'
   file_list = os.listdir(dir1)
   print file_list
   dir2= [x for x in file_list if '.jpg' in x]
   print dir2

   flist = []
   for x in dir2:
      t = read_tensor_from_image_file(os.path.join(dir1,x),
                                       input_height=input_height,
                                       input_width=input_width)
      flist.append(t)

   while 1 == 1: 
      for x in flist:
         time1 = time.time()
         results = testgd.run(x)
         time2 = time.time()
         results *= 240
         print 'function took %0.3f ms' % ((time2-time1)*1000.0)
         #print results

         time1 = time.time()
         results1 = testgd1.run(flist[0])
         time2 = time.time()
         results1 *= 240
         print '2nd function took %0.3f ms' % ((time2-time1)*1000.0)
         #print dir2[0]
         print results1
