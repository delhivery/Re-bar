# Minimum Required Version of CMake
cmake_minimum_required(VERSION 2.8.9)

# Projct being built
project(marge)

# Set include directories for header files
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(MARGE_SRCS
    ${PROJECT_SOURCE_DIR}/graph.cxx
    ${PROJECT_SOURCE_DIR}/optimal.cxx
    ${PROJECT_SOURCE_DIR}/pareto.cxx
)

include_directories("${PROJECT_BINARY_DIR}")

add_library(${PROJECT_NAME} SHARED ${MARGE_SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIR})

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

file(GLOB HEADER_FILES_AND_DIRS "${PROJECT_INCLUDE_DIR}/*")

foreach(ITEM ${HEADER_FILES_AND_DIRS})
    IF(IS_DIRECTORY "${ITEM}")
        LIST(APPEND DIRS_TO_INSTALL "${ITEM}")
    ELSE()
        LIST(APPEND FILES_TO_INSTALL "${ITEM}")
    ENDIF()
endforeach()

install(FILES ${FILES_TO_INSTALL} DESTINATION ${CMAKE_INSTALL_OLDINCLUDEDIR}/${PROJECT_NAME})
install(DIRECTORY ${DIRS_TO_INSTALL} DESTINATION ${CMAKE_INSTALL_OLDINCLUDEDIR}/${PROJECT_NAME})
