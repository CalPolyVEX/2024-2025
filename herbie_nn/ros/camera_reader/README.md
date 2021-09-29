# camera_reader

This ROS node captures images from the camera, reduces the image size to 640x360, and runs the image through the neural network.  The neural network output data
is published to the topic /nn_data.
