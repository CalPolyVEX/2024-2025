#run this file to copy the necessary library and include files
#to run tensorflow using C++
#
#!/bin/bash

#this is the directory where tensorflow was compiled
tf_dir=/mnt/temp/tensorflow
echo $tf_dir

#copy the tensorflow library
#mkdir lib
#cp $tf_dir/bazel-bin/tensorflow/libtensorflow_cc.so ./lib
#cp $tf_dir/bazel-bin/tensorflow/libtensorflow_framework.so ./lib

#copy the include files
mkdir include
#cp -r $tf_dir/bazel-genfiles/* ./include/
#cp -r $tf_dir/tensorflow/cc ./include/tensorflow
#cp -r $tf_dir/tensorflow/core ./include/tensorflow
cp -r $tf_dir/third_party ./include/

#link to protobuf (protobuf needs to be installed in the home directory)
ln -s /home/jseng/protobuf/src/google include/google

#copy eigen 3 include files
cp -r /usr/include/eigen3/* ./include

#absl link
ln -s $tf_dir/absl ./include/absl
