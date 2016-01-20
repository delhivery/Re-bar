# Minimum Required Version of CMake
cmake_minimum_required(VERSION 2.8.9)

# Projct being built
project(rebar)

# Set binary dir
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

# Set executable and library output paths
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Set include directories for header files
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

include_directories("${PROJECT_INCLUDE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

# Default build type for this build
set(CMAKE_BUILD_TYPE Release)

# Build flags for build types
set(CMAKE_CXX_FLAGS "-std=c++14")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -pipe -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -pipe -march=native")

add_subdirectory(marge)
add_subdirectory(jezik)
add_subdirectory(rebar)
