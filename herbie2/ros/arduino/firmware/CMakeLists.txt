cmake_minimum_required(VERSION 2.8.3)

include_directories(${ROS_LIB_DIR})

# Remove this if using an Arduino without native USB (eg, other than Leonardo)
add_definitions(-DUSB_CON)

generate_arduino_firmware(jet_binary
  #uncomment the following line to use rosserial
  #SRCS sample_LS7366R.cpp ${ROS_LIB_DIR}/time.cpp

  #do not use the rosserial library, send the encoder values directly
  SRCS test_LS7366R.cpp 
  BOARD uno
  PORT /dev/arduino
)

