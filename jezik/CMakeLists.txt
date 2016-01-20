# Minimum Required Version of CMake
cmake_minimum_required(VERSION 2.8.9)

# Projct being built
project(jezik)

# Set include directories for header files
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(JEZIK_SRCS
    ${PROJECT_SOURCE_DIR}/jezik.cxx
)

include_directories("${PROJECT_BINARY_DIR}")

add_library(${PROJECT_NAME} STATIC ${JEZIK_SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIR})
