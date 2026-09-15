# Fisheye Image Undistortion Tool

This tool performs batch undistortion of fisheye camera images using OpenCV.

## Requirements
- C++17 compatible compiler
- CMake (version 3.10 or higher)
- OpenCV library

## Building the project
```bash
mkdir build
cd build
cmake ..
make
```

## Usage
```bash
./fisheye_undistort <input_folder> <output_folder>
```

Example:
```bash
./fisheye_undistort ~/images/distorted ~/images/undistorted
```

## Configuration
Camera parameters are loaded from `config/camera_intrinsics.yaml`. The file should contain:
- `camera_matrix`: 3x3 camera intrinsic matrix
- `distortion_coefficients`: Vector of distortion coefficients

The program supports common image formats including JPG, PNG, BMP, and TIFF.
