cmake_minimum_required(VERSION 2.8.3)
project(navigation_goals)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

find_package(catkin REQUIRED COMPONENTS
  roscpp
  roslint
  move_base_msgs
  actionlib
  tf2_ros
  std_msgs
  geometry_msgs
  tf2_sensor_msgs
  message_generation
)

find_package(Boost REQUIRED COMPONENTS system thread chrono)

roslint_cpp()

# generate_dynamic_reconfigure_options(
#   cfg/LookupTransform.cfg
# )

catkin_package(
  CATKIN_DEPENDS roscpp tf2_ros move_base_msgs actionlib geometry_msgs std_msgs tf2_sensor_msgs message_generation
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

add_executable(navigation_goals
  src/navigation_goals.cpp)

#add_dependencies(navigation_goals ${PROJECT_NAME}_gencfg)
target_link_libraries(navigation_goals ${catkin_LIBRARIES})

