cmake_minimum_required(VERSION 2.8.3)
project(navigation_goals)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

find_package(catkin REQUIRED COMPONENTS
  dynamic_reconfigure
  roscpp
  roslint
  move_base_msgs
  actionlib
)

roslint_cpp()

# generate_dynamic_reconfigure_options(
#   cfg/LookupTransform.cfg
# )

catkin_package(
  CATKIN_DEPENDS roscpp tf2_ros move_base_msgs actionlib 
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

add_executable(navigation_goals
  src/navigation_goals.cpp)

#add_dependencies(navigation_goals ${PROJECT_NAME}_gencfg)
target_link_libraries(navigation_goals ${catkin_LIBRARIES})

