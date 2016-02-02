# Minimum Required Version of CMake
cmake_minimum_required(VERSION 2.8.9)

# Projct being built
project(fletcher)

# Set include directories for header files
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(REBAR_SRCS
    ${PROJECT_SOURCE_DIR}/main.cxx
)

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_INCLUDE_DIR}")

find_package(Threads)

add_executable(${PROJECT_NAME} ${REBAR_SRCS})
target_link_libraries(${PROJECT_NAME} marge jezik ${CMAKE_THREAD_LIBS_INIT})
