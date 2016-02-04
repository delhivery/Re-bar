# Minimum Required Version of CMake
cmake_minimum_required(VERSION 2.8.9)

# Projct being built
project(rebar CXX)

include(GNUInstallDirs)

if ("${CMAKE_COMPILER_IS_GNUCXX}")
    if ("${CMAKE_CXX_COMPILER_VERSION}" MATCHES "5\\.3\\.[0-9]")
        message(STATUS "${GREEN}Supported CXX compiler detected.${RESET}")
    else ()
        message(FATAL_ERROR "Your compiler is too old. ${CMAKE_CXX_COMPILER_VERSION} detected. 5.3.0 or greater required")
    endif()
else ()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

find_package (Boost 1.48.0 REQUIRED COMPONENTS graph)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message( STATUS "${GREEN}Found BGL include at:${Boost_INCLUDE_DIRS}${RESET}" )
else ()
    message( FATAL_ERROR "${RED}Failed to locate bgl.${RESET}" )
endif()

find_package (Doxygen)

if (Doxygen_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc ${DOXYGEN_EXECUTABLE}
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generating API documentation with
        Doxygen" VERBATIM)
endif()

find_path(asio_INCLUDE asio.hpp HINTS "/usr/include" "/usr/local/include" "/opt/local/include" )

if (asio_INCLUDE)
    include_directories(${asio_INCLUDE})
    message( STATUS "${Green}Found ASIO include at: ${asio_INCLUDE}${RESET}" )
else ()
    message( FATAL_ERROR "${RED}Failed to locate ASIO dependency.${RESET}" )
endif()

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
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -O0 -g -pipe -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -pipe -march=native")

add_subdirectory(marge)
add_subdirectory(jezik)
add_subdirectory(rebar)
