cmake_minimum_required(VERSION 2.8.3)
project(nav_herbie)
#set(CMAKE_CXX_FLAGS "-g -Wall -Wextra")
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
  pcl_conversions
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
  /usr/local/include/igraph
)

add_executable(navigation
  src/navigation.cpp
  src/graph.cpp
  src/move_base.cpp
  src/control_board.cpp
  src/recovery.cpp
  src/navigation.h)

add_dependencies(navigation ${PROJECT_NAME}_gencfg)
target_link_libraries(navigation ${catkin_LIBRARIES} -ligraph -L/usr/local/lib)
#target_include_directories (${JS_OPENCV_V4L2_BIN} PUBLIC ${V4L2_HELPER_LIB_INCLUDE_DIR})
#target_link_libraries (${JS_OPENCV_V4L2_BIN} v4l2_helper ${catkin_LIBRARIES})
