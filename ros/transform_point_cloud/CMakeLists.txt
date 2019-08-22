cmake_minimum_required(VERSION 2.8.3)
project(transform_point_cloud)
set(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra -pedantic")

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

add_executable(transform_point_cloud
  src/transform_point_cloud)
add_executable(pub_odometry
  src/pub_odometry.cpp
  src/encoder_odom.cpp
  src/encoder_odom.h)
add_executable(transform_gyro_msgs
  src/transform_gyro_msgs.cpp)

add_executable(bno055
  src/bno055.cpp
  src/bno055.h)

add_dependencies(transform_point_cloud ${PROJECT_NAME}_gencfg)
target_link_libraries(transform_point_cloud ${catkin_LIBRARIES})

add_dependencies(pub_odometry ${PROJECT_NAME}_gencfg)
target_link_libraries(pub_odometry ${catkin_LIBRARIES})

add_dependencies(transform_gyro_msgs ${PROJECT_NAME}_gencfg)
target_link_libraries(transform_gyro_msgs ${catkin_LIBRARIES})

add_dependencies(bno055 ${PROJECT_NAME}_gencfg)
target_link_libraries(bno055 ${catkin_LIBRARIES})
