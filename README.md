# ue4
Joystick controls

* button 1 press - toggle manual control
* button 1 hold - exit ROS
* button 2 hold - start/stop recording .bag file
* button 3 press - toggle manual/autonomous driving
* button 4 press - send navigation goal

Sparse checkout for ROS (this will checkout only the ROS directory into the catkin_ws/src directory)

```mkdir myrepo
cd catkin_ws/src
git init
git config core.sparseCheckout true
echo 'ros/' > .git/info/sparse-checkout
git remote add -f origin https://github.com/jsseng/ue4.git
git pull origin master
```

Extra packages to install

```
apt install python-pygame
apt install ros-kinetic-rosserial-arduino
apt install ros-kinetic-tf2-sensor-msgs
```
