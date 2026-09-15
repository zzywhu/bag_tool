#!/bin/bash

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}/bag_to_images_cpp"
BUILD_DIR="${PROJECT_DIR}/build"

# Ensure we have ROS environment
if [ ! -f "/opt/ros/noetic/setup.bash" ]; then
    echo "Error: ROS Noetic not found. Please install ROS first."
    exit 1
fi

source /opt/ros/noetic/setup.bash

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Build the project directly with CMake instead of catkin
echo "Building the project..."
cmake .. && make -j$(nproc)

# Create a symbolic link to the executable in the main directory
if [ -f "${BUILD_DIR}/bag_to_images" ]; then
    ln -sf "${BUILD_DIR}/bag_to_images" "${SCRIPT_DIR}/bag_to_images"
    echo "Executable linked to: ${SCRIPT_DIR}/bag_to_images"
else
    echo "Error: Build succeeded but executable not found at expected location."
    exit 1
fi

echo ""
echo "Build completed successfully!"
echo "Usage: ${SCRIPT_DIR}/bag_to_images <bag_file> <image_topic> [output_dir]"
echo ""
echo "Example for compressed images:"
echo "${SCRIPT_DIR}/bag_to_images /path/to/your.bag /camera/image_raw/compressed ~/extracted_images"
