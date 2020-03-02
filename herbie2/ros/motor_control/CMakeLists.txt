cmake_minimum_required(VERSION 2.8.3)
project(motor_control)
#set(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra -pedantic")
set(CMAKE_CXX_FLAGS "")

find_package(catkin REQUIRED COMPONENTS
  dynamic_reconfigure
  geometry_msgs
  roscpp
  roslint
  tf2_py
  tf2_ros
  tf2_sensor_msgs
  sensor_msgs
  serial
)

roslint_cpp()

generate_dynamic_reconfigure_options(
  cfg/LookupTransform.cfg
)

catkin_package(
  CATKIN_DEPENDS roscpp tf2_ros dynamic_reconfigure
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

add_executable(pub_odometry
  src/pub_odometry.cpp
  src/encoder_odom.cpp
  src/encoder_odom.h)

add_dependencies(pub_odometry ${PROJECT_NAME}_gencfg)
target_link_libraries(pub_odometry ${catkin_LIBRARIES})

