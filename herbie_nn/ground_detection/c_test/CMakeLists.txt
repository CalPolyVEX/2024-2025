if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(pytorch_inference)

set(Torch_DIR /home/jseng/pt-env/lib/python3.6/site-packages/torch/share/cmake/Torch)
find_package(Torch REQUIRED)

add_executable(example-app test.cpp)
target_link_libraries(example-app nvinfer nvinfer_plugin nvcaffe_parser nvonnxparser)
target_link_libraries(example-app "${TORCH_LIBRARIES}")
set_property(TARGET example-app PROPERTY CXX_STANDARD 14)
