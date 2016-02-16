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

# install(TARGETS ${PROJECT_NAME} DESTINATION bin)
# 
# file(GLOB HEADER_FILES_AND_DIRS "${PROJECT_INCLUDE_DIR}/*")
# 
# foreach(ITEM ${HEADER_FILES_AND_DIRS})
#     IF(IS_DIRECTORY "${ITEM}")
#         LIST(APPEND DIRS_TO_INSTALL "${ITEM}")
#     ELSE()
#         LIST(APPEND FILES_TO_INSTALL "${ITEM}")
#     ENDIF()
# endforeach()
# 
# install(FILES ${FILES_TO_INSTALL} DESTINATION include/${PROJECT_NAME})
# install(DIRECTORY ${DIRS_TO_INSTALL} DESTINATION include/${PROJECT_NAME})
