cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(custom_ops)

set(Torch_DIR /home/jseng/pt-env/lib/python3.6/site-packages/torch/share/cmake/Torch)
find_package(Torch REQUIRED)
add_executable(example-app test.cpp)
target_link_libraries(example-app "${TORCH_LIBRARIES}")
set_property(TARGET example-app PROPERTY CXX_STANDARD 14)
