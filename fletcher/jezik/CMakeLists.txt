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

file(GLOB HEADER_FILES_AND_DIRS "${PROJECT_INCLUDE_DIR}/*")

foreach(ITEM ${HEADER_FILES_AND_DIRS})
    IF(IS_DIRECTORY "${ITEM}")
        LIST(APPEND DIRS_TO_INSTALL "${ITEM}")
    ELSE()
        LIST(APPEND FILES_TO_INSTALL "${ITEM}")
    ENDIF()
endforeach()

install(FILES ${FILES_TO_INSTALL} DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${DIRS_TO_INSTALL} DESTINATION include/${PROJECT_NAME})
