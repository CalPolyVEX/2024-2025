#!/usr/bin/env python
from __future__ import print_function

import roslib
roslib.load_manifest('image_test')
import sys, rospy, cv2, time, imp
import numpy as np
import tensorflow as tf
from solvepnp import camera_transform
from std_msgs.msg import String
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError
from image_test.msg import ground_boundary

class GroundDetector:
   def __init__(self, protobuf_model, input_layer, output_layer):
      self.width = 480
      self.height = 270
      self.protobuf_model = protobuf_model
      self.input_layer = input_layer
      self.output_layer = output_layer
      self.input_name = "import/" + input_layer
      self.output_name = "import/" + output_layer
      self.counter=0

      self.point_filter_list = []
      for i in range(48):
         self.k = cv2.KalmanFilter(1, 1, 0, cv2.CV_32F)

         #create control matrix B
         #self.k.controlMatrix = self.b
         self.k.measurementMatrix = np.ones((1,1))
         self.k.transitionMatrix = np.ones((1,1))

         # 1/270 = .0037
         self.k.measurementNoiseCov = .002 * np.ones((1,1))
         self.k.processNoiseCov= .001 * np.ones((1,1))

         self.k.statePost = .10 * np.ones((1,1))
         self.k.errorCovPost = .10 * np.ones((1,1))

         self.point_filter_list.append(self.k)

      #needed to prevent memory errors on the Jetson TX2
      config = tf.compat.v1.ConfigProto()
      config.gpu_options.allow_growth = True

      #load the graph
      self.load_graph()
      self.input_operation = self.graph.get_operation_by_name(self.input_name);
      self.output_operation = self.graph.get_operation_by_name(self.output_name);
      self.sess = tf.compat.v1.Session(config=config,graph=self.graph)

      #load the second graph
      if 1==0:
         self.load_graph1()
         self.input_operation1 = self.graph1.get_operation_by_name(self.input_name);
         self.output_operation1 = self.graph1.get_operation_by_name(self.output_name);
         self.sess1 = tf.Session(config=config,graph=self.graph1)

   def load_graph(self):
      self.graph = tf.compat.v1.Graph()
      self.graph_def = tf.compat.v1.GraphDef()

      with open(self.protobuf_model, "rb") as f:
         self.graph_def.ParseFromString(f.read())
      with self.graph.as_default():
         tf.import_graph_def(self.graph_def)

   def load_graph1(self):
      self.graph1 = tf.Graph()
      self.graph_def1 = tf.GraphDef()

      #with open(self.protobuf_model, "rb") as f:
      with open('/home/nvidia/catkin_ws/src/ros/image_test/nodes/output_graph_save.pb', "rb") as f:
         self.graph_def1.ParseFromString(f.read())
      with self.graph1.as_default():
         tf.import_graph_def(self.graph_def1)

   def run(self, input_image):
      results = self.sess.run(self.output_operation.outputs[0], {self.input_operation.outputs[0]: input_image})

      if 1==0:
         #kalman filter the output points
         output = np.zeros(shape=(48,1))
         for i in range(48):
            temp_k = self.point_filter_list[i]

            output[i,0] = temp_k.predict().item(0)
            #rospy.loginfo("%s", temp_k.predict())
            #rospy.loginfo(results[i].item(0))
            #temp_k.processNoiseCov= (.001 + output[i,0]*.008) * np.ones((1,1))
            #if i < 24:
               #temp_k.measurementNoiseCov = (.001 + output[i,0]*.008) * np.ones((1,1))
               #temp_k.measurementNoiseCov = (.001 + ((24-i)/24.0)*.008) * np.ones((1,1))
            #temp_k.processNoiseCov= .008 * np.ones((1,1))
            #if i==0:
            #   rospy.loginfo(output[i,0])
            temp_k.correct(np.array([results[i].item(0)]))

         #rospy.loginfo(output)
         #rospy.loginfo(results)
         return output
      return results

      #rospy.loginfo("%s", output)


   def run1(self, input_image):
      results1 = self.sess1.run(self.output_operation1.outputs[0], {self.input_operation1.outputs[0]: input_image})

      return results1

class image_converter:
   def __init__(self, infer_file):
      self.input_layer='input_1'
      self.output_layer='k2tfout_0'

      self.gd = GroundDetector(infer_file, self.input_layer, self.output_layer)

      self.counter = 0
      self.image_pub = rospy.Publisher("/see3cam_cu20/test_image_topic_3",Image, queue_size=1)
      self.image_resized_pub = rospy.Publisher("/see3cam_cu20/image_raw_480_270",Image, queue_size=1)

      self.bridge = CvBridge()
      self.image_sub = rospy.Subscriber("/see3cam_cu20/image_raw",Image,self.callback)

      self.msgpub = rospy.Publisher('/point_array', ground_boundary, queue_size=1)
      self.camera_t = camera_transform()

   def callback(self,data):
      try:
         cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
      except CvBridgeError as e:
         print(e)

      #save the file
      (rows,cols,channels) = cv_image.shape

      #resize and convert the image to numpy array
      resized_image_nn = cv2.resize(cv_image, (480, 270))
      #resized_image = resized_image_nn.copy()
      np_image_data = np.asarray(resized_image_nn)
      #float_caster = tf.cast(np_image_data, tf.float32)
      float_caster = np_image_data / 255.0
      np_final = np.expand_dims(float_caster,axis=0)
      #print (np_final.shape)

      #get the neural network computation time
      time1 = time.time()
      results = self.gd.run(np_final)
      results *= 270.0

      #results1 = self.gd.run1(np_final)
      #results1 *= 270.0
      time2 = time.time()

      #send the point array message
      t = ground_boundary()
      temp_x = []
      temp_y = []
      column=5
      for x in results:
         temp_point_list = self.camera_t.compute(column,x)
         #temp.append(int(x))
         temp_x.append(temp_point_list[0])
         temp_y.append(temp_point_list[1])
         column += 10
      t.point_x = temp_x
      t.point_y = temp_y

      column = 5

      font = cv2.FONT_HERSHEY_SIMPLEX
      fps = 1 / ((time2-time1))
      cv2.putText(resized_image_nn, "%.2ffps" % fps, (390, 20), font, 0.6, (0, 255, 0), 1, cv2.LINE_AA)

      #stats
      #if ((time2-time1) > .070):
      #   self.counter += 1
      #cv2.putText(resized_image_nn, "%d" % self.counter, (390, 40), font, 0.6, (0, 255, 0), 1, cv2.LINE_AA)

      for x in results:
         cv2.circle(resized_image_nn, (int(column),int(x)), 2, (0,0,255), 3)
         column += 10

      column = 5

      if 1==0:
         for x in results1:
            cv2.circle(resized_image_nn, (int(column),int(x)), 2, (0,255,0), 3)
            column += 10

      try:
         self.image_pub.publish(self.bridge.cv2_to_imgmsg(resized_image_nn, "bgr8"))
         #self.image_resized_pub.publish(self.bridge.cv2_to_imgmsg(resized_image, "bgr8"))
         self.msgpub.publish(t)
      except CvBridgeError as e:
         print(e)

def main(args):
   ic = image_converter(args[1])
   rospy.init_node('infer_node', anonymous=True)
   try:
      rospy.spin()
   except KeyboardInterrupt:
      print("Shutting down")
   cv2.destroyAllWindows()

if __name__ == '__main__':
   main(sys.argv)
