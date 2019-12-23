Steps to compile tensorflow for use with ROS:
 * install Bazel (v0.15.0)
   https://github.com/bazelbuild/bazel/releases/download/0.15.0/bazel-0.15.0-dist.zip
   * sudo apt-get install build-essential openjdk-8-jdk python zip unzip
   * env EXTRA_BAZEL_ARGS="--host_javabase=@local_jdk//:jdk" bash ./compile.sh
 * git clone https://github.com/tensorflow/tensorflow.git
 * cd tensorflow
 * git checkout v1.12.0
 * ./configure (Jetpack 4.2 = CUDA 10.0, CuDNN 7.3.1, TX2 compute capability 6.2)
 * bazel build -c opt --config=monolithic //tensorflow:libtensorflow_cc.so (refer to https://medium.com/@tomdeore/standalone-c-build-tensorflow-opencv-6dc9d8a1412d)
   * if you get a AWS linking error:  https://github.com/tensorflow/serving/issues/832
 * copy the library files to a lib directory:
   * mkdir ../my_project/lib
   * cp bazel-bin/tensorflow/libtensorflow_cc.so /home/jseng/catkin_ws/src/ros/image_test/lib
   * cp bazel-bin/tensorflow/libtensorflow_framework.so /home/jseng/catkin_ws/src/ros/image_test/lib
 * copy the include files:
   * cp -r bazel-genfiles/* /home/jseng/catkin_ws/src/ros/image_test/include/
   * cp -r tensorflow/cc /home/jseng/catkin_ws/src/ros/image_test/include/tensorflow
   * cp -r tensorflow/core /home/jseng/catkin_ws/src/ros/image_test/include/tensorflow
   * cp -r third_party /home/jseng/catkin_ws/src/ros/image_test/include/
 * install protobuf 3.6.0
   * git clone https://github.com/protocolbuffers/protobuf.git
   * cd protobuf
   * git checkout v3.6.0
   * git submodule update --init --recursive
   * ./autogen.sh
 * create a symbolic link in the include directory to the protobuf include directory
   * ln -s /home/jseng/protobuf/src/google google 
 * copy all the Eigen include files
   * cd /usr/include/eigen3
   * cp -r * /home/jseng/catkin_ws/src/ros/image_test/include
 * absl header error
   * git clone absl into the tensorflow directory
     * cd /home/jseng/tensorflow
     * git clone https://github.com/abseil/abseil-cpp.git
     * ln -s abseil-cpp/absl ./absl
   * create a link from 'include/absl' to 'tensorflow/absl'
     * cd /home/jseng/catkin_ws/src/ros/image_test/include
     * ln -s /home/jseng/tensorflow/absl absl
   * (https://github.com/tensorflow/tensorflow/issues/22007#issuecomment-424553600)
