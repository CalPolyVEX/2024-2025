This directory contains the ROS files that should be placed in the src directory of the catkin workspace on the Jetson.

# Building the ROS files

Assuming a clean linux install on the x86 computer, these are the steps to get an install that will build using ```catkin_make```.  You may get an 
error with the configuration looking for CUDA.  If that is not installed, you may need to delete the camera_reader node.

1.  Create a python virtual environment (pt-env) and install PyTorch into that environment:
* ```python3 -m venv pt-env```
* ```source pt-env/bin/activate```
* ```pip3 install torch```
2.  Add the following lines to your .bashrc:  
* ```export Torch_DIR=/home/student/pt-env/lib64/python3.8/site-packages/torch/share/cmake/Torch```
3.  Create a new workspace:  ```mkdir -p ~/catkin_ws/src```
4.  Change to the workspace:  ```cd ~/catkin_ws/src```
5.  Clone the serial library into the workspace source (catkin_ws/src):  ```git clone https://github.com/wjwwood/serial.git```
6.  Checkout this directory into the workspace source directory (catkin_ws/src):
* ```git init```
* ```git config core.sparseCheckout true```
* ```echo 'herbie_nn/ros/' > .git/info/sparse-checkout```
* ```git remote add -f origin https://github.com/jsseng/ue4.git```
* ```git pull origin master```
7.  Change to ```~/catkin_ws``` and run ```catkin_make```
