cmake_minimum_required(VERSION 3.8)
project(astro_move)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -lserial)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

# Generate Message
rosidl_generate_interfaces(${PROJECT_NAME} "msg/MotorControlMSG.msg")

# Build Executable
add_executable(astro_move_ src/astro_move.cpp)
ament_target_dependencies(astro_move_ rclcpp std_msgs)
add_executable(motor_control ./../../../packet_computer/buildmessage.cpp src/motor_control.cpp)
ament_target_dependencies(motor_control rclcpp std_msgs)

# Bind Message
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(astro_move_ "${cpp_typesupport_target}") 
target_link_libraries(motor_control "${cpp_typesupport_target}" serial) 

# Install
install(TARGETS astro_move_ motor_control DESTINATION lib/${PROJECT_NAME})

ament_package()
