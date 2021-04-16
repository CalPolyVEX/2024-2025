cmake_minimum_required (VERSION 3.0)
message("CMAKE_C_FLAGS_DEBUG is ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_RELEASE is ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_C_FLAGS_RELWITHDEBINFO is ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
message("CMAKE_C_FLAGS_MINSIZEREL is ${CMAKE_C_FLAGS_MINSIZEREL}")

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")

project ("camera_reader" ASM)

set (V4L2_SOURCE "src/opencv_v4l2.cpp")
set (JS_V4L2_SOURCE "src/js_opencv_v4l2.cpp")

#check if the CPU is ARM
if ("x86_64" STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
   set (JS_V4L2_SOURCE_ASM "")
else()
   set (JS_V4L2_SOURCE_ASM "src/test_asm.S")
endif()

set (MAIN_SOURCE "src/opencv_main.cpp")
set (INFO_SOURCE "src/opencv_buildinfo.cpp")

set (OPENCV_V4L2_BIN "opencv-v4l2")
set (JS_OPENCV_V4L2_BIN "camera_reader_node")
set (OPENCV_BUILDINFO_BIN "opencv-buildinfo")
set_property(SOURCE "src/test_asm.S" PROPERTY LANGUAGE C)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

find_package(catkin REQUIRED COMPONENTS
  roscpp
  roslint
  image_transport
  cv_bridge
)

roslint_cpp()

catkin_package(
  CATKIN_DEPENDS roscpp image_transport cv_bridge
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

# Include the directories containing libraries
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/lib")
add_subdirectory (lib)

set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${V4L2_HELPER_LIB_INSTALL_PATH}")

# Specify the compiler flags
set (GCC_COMPILE_FLAGS -Wall -Wpedantic -Wextra -O3 -Wshadow -std=c++11 -g -static)
add_compile_options (${GCC_COMPILE_FLAGS})

if ("x86_64" STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
   add_executable (${JS_OPENCV_V4L2_BIN} ${JS_V4L2_SOURCE})
else()
   add_executable (${JS_OPENCV_V4L2_BIN} ${JS_V4L2_SOURCE} ${JS_V4L2_SOURCE_ASM})
   target_compile_definitions(${JS_OPENCV_V4L2_BIN} PUBLIC ARM_CPU)
endif()

target_include_directories (${JS_OPENCV_V4L2_BIN} PUBLIC ${V4L2_HELPER_LIB_INCLUDE_DIR})
target_link_libraries (${JS_OPENCV_V4L2_BIN} v4l2_helper ${catkin_LIBRARIES})
target_link_libraries (${JS_OPENCV_V4L2_BIN} ${OpenCV_LIBS})

add_executable (${OPENCV_V4L2_BIN} ${V4L2_SOURCE})
target_include_directories (${OPENCV_V4L2_BIN} PUBLIC ${V4L2_HELPER_LIB_INCLUDE_DIR})
target_link_libraries (${OPENCV_V4L2_BIN} v4l2_helper)
target_link_libraries (${OPENCV_V4L2_BIN} ${OpenCV_LIBS})

install (
	TARGETS
	#${OPENCV_V4L2_BIN}
	RUNTIME DESTINATION bin
)

# uninstall target
# Ref: https://gitlab.kitware.com/cmake/community/wikis/FAQ#can-i-do-make-uninstall-with-cmake
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
