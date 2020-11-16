Steps to compile tensorflow (v2.3.1) for use with ROS:
 * install Bazel (v3.1.0)
   https://github.com/bazelbuild/bazel/releases/download/3.1.0/bazel-3.1.0-dist.zip
   * sudo apt-get install build-essential openjdk-8-jdk python zip unzip
   * env EXTRA_BAZEL_ARGS="--host_javabase=@local_jdk//:jdk" bash ./compile.sh
 * compile libnccl
   * git clone https://github.com/NVIDIA/nccl.git
   * git checkout v2.6.4-1
   * make
   * make install
 * compile tensorflow
   * git clone https://github.com/tensorflow/tensorflow.git
   * cd tensorflow
   * git checkout v2.3.1
   * ./configure (Jetpack 4.4.1 = CUDA 10.2, CuDNN 8.0, Xavier compute capability 7.2)
   * bazel build -c opt --config=monolithic //tensorflow:libtensorflow_cc.so (refer to https://medium.com/@tomdeore/standalone-c-build-tensorflow-opencv-6dc9d8a1412d)
 * copy the library files to a lib directory:
   * mkdir ../my_project/lib
   * cp bazel-bin/tensorflow/libtensorflow_cc.so /home/jseng/catkin_ws/src/ros/image_cnn/lib
 * copy the include files:
   * cp -r bazel-genfiles/* /home/jseng/catkin_ws/src/ros/image_cnn/include/
   * cp -r bazel-bin/tensorflow/cc /home/jseng/catkin_ws/src/ros/image_cnn/include/tensorflow
   * cp -r bazel-bin/tensorflow/core /home/jseng/catkin_ws/src/ros/image_cnn/include/tensorflow
   * cp -r tensorflow/cc /home/jseng/catkin_ws/src/ros/image_cnn/include/tensorflow
   * cp -r tensorflow/core /home/jseng/catkin_ws/src/ros/image_cnn/include/tensorflow
   * cp -r third_party /home/jseng/catkin_ws/src/ros/image_cnn/include/
 * install protobuf 3.9.0
   * git clone https://github.com/protocolbuffers/protobuf.git
   * cd protobuf
   * git checkout v3.9.0
   * git submodule update --init --recursive
   * ./autogen.sh
 * create a symbolic link in the include directory to the protobuf include directory
   * ln -s /home/jseng/protobuf/src/google google 
 * copy all the Eigen include files
   * cd /usr/include/eigen3
   * cp -r * /home/jseng/catkin_ws/src/ros/image_cnn/include
 * absl header error
   * git clone absl into the tensorflow directory
     * cd /home/jseng/tensorflow
     * git clone https://github.com/abseil/abseil-cpp.git
     * ln -s abseil-cpp/absl ./absl
   * create a link from 'include/absl' to 'tensorflow/absl'
     * cd /home/jseng/catkin_ws/src/ros/image_cnn/include
     * ln -s /home/jseng/tensorflow/absl absl
   * (https://github.com/tensorflow/tensorflow/issues/22007#issuecomment-424553600)
