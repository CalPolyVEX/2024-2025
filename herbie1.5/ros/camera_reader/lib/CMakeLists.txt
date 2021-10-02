cmake_minimum_required (VERSION 3.0)

set (V4L2_HELPER_LIB_VERSION_MAJOR 0)
set (V4L2_HELPER_LIB_VERSION_MINOR 1)
set (V4L2_HELPER_LIB_VERSION_PATCH 0)
set (
	V4L2_HELPER_LIB_VERSION_STRING
	${V4L2_HELPER_LIB_VERSION_MAJOR}.${V4L2_HELPER_LIB_VERSION_MINOR}.${V4L2_HELPER_LIB_VERSION_PATCH}
)
set (V4L2_HELPER_LIB_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/lib" CACHE INTERNAL "")
set (V4L2_HELPER_HEADER_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/include" CACHE INTERNAL "")
set (V4L2_HELPER_LIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/inc" CACHE INTERNAL "")

set (GCC_COMPILE_FLAGS -Wall -Wpedantic -Wextra -O3 -Wshadow -g)
add_compile_options (${GCC_COMPILE_FLAGS})

add_library (v4l2_helper SHARED src/v4l2_helper.c)
target_include_directories (v4l2_helper PUBLIC ${V4L2_HELPER_LIB_INCLUDE_DIR})

set_target_properties (
	v4l2_helper PROPERTIES
	VERSION ${V4L2_HELPER_LIB_VERSION_STRING}
	SOVERSION ${V4L2_HELPER_LIB_VERSION_MAJOR}.${V4L2_HELPER_LIB_VERSION_MINOR} # Number that updates for changes to the ABI
)
install (TARGETS v4l2_helper LIBRARY DESTINATION ${V4L2_HELPER_LIB_INSTALL_PATH})
install (FILES ${V4L2_HELPER_LIB_INCLUDE_DIR}/v4l2_helper.h DESTINATION ${V4L2_HELPER_HEADER_INSTALL_PATH})

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
