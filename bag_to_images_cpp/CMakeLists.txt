cmake_minimum_required(VERSION 3.0.2)
project(bag_to_images)

# Find ROS packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rosbag
  cv_bridge
  sensor_msgs
  std_msgs
  image_transport
)

# Find system libraries
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system)

# Include directories
include_directories(
  ${catkin_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

# Set output directory to build directory rather than devel
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Declare executables
add_executable(bag_to_images src/bag_to_images.cpp)
target_link_libraries(bag_to_images
  ${catkin_LIBRARIES}
  ${OpenCV_LIBRARIES}
  ${Boost_LIBRARIES}
)
