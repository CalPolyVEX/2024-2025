cmake_minimum_required(VERSION 2.8.3)
project(navigation)
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
  std_msgs
  image_transport
  cv_bridge
)

roslint_cpp()

#generate_dynamic_reconfigure_options(
#  cfg/LookupTransform.cfg
#)

catkin_package(
  CATKIN_DEPENDS roscpp tf2_ros dynamic_reconfigure 
)

include_directories(
  ${catkin_INCLUDE_DIRS} 
)

add_executable(navigation
  src/navigation.cpp
  src/navigation.h)

add_dependencies(navigation ${PROJECT_NAME}_gencfg)
target_link_libraries(navigation ${catkin_LIBRARIES})
