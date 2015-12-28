cmake_minimum_required(VERSION 3.0.2)
project(xform_mobile)

# Extra CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Additional CMake functions
include(helpers)


#######################################################################
#                            Global flags                             #
#######################################################################

set(CMAKE_BUILD_TYPE release)
set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS}   -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_DEBUG}   -O0 -Wall -g -DXDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_RELEASE} -O3 -DNDEBUG")


#######################################################################
#                         External Libraries                          #
#######################################################################

find_package(Boost COMPONENTS system filesystem REQUIRED)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

find_package(ZLIB REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIRS})

# Halide
set(HALIDE_INCLUDES $ENV{HALIDE}/include/)
set(HALIDE_LIBRARY $ENV{HALIDE}/bin/libHalide.so)
include_directories(${HALIDE_INCLUDES})
message("HalideL lib" ${HALIDE_LIBRARY})

# Android
set(ANDROID_ABI armeabi-v7a)
set(HL_TARGET "arm-32-android-armv7s")
message("Android ABI: ${ANDROID_ABI}")


#######################################################################
#                                Path                                 #
#######################################################################

# Directories
set(bin "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(inc "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(obj "${CMAKE_CURRENT_SOURCE_DIR}/obj")
set(lib "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(src "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(3rdparty "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

set(pywrapper "${CMAKE_CURRENT_SOURCE_DIR}/lib")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${lib})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${bin})

include_directories(${inc})
add_subdirectory(${src})


#######################################################################
#                               Testing                               #
#######################################################################
 
set(GTEST_ROOT ${3rdparty}/gtest CACHE STRING "")

enable_testing()
add_subdirectory(test)
