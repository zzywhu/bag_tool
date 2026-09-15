cmake_minimum_required(VERSION 3.10)
project(fisheye_undistort)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Add executable
add_executable(fisheye_undistort src/fisheye_undistort.cpp)

# Link libraries
target_link_libraries(fisheye_undistort ${OpenCV_LIBS})

# Print OpenCV version information
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
