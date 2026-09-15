BAG TO IMAGES EXTRACTION TOOL
==============================

This tool extracts images from ROS bag files and saves them as PNG files, named with their ROS timestamps.

Features:
- Supports both regular Image and CompressedImage message types
- Automatically detects image format (compressed or uncompressed)
- Preserves original image quality
- Uses timestamp-based filenames (format: seconds.nanoseconds.png) for accurate sequencing
  Example: 1636531400.998912096.png
- Supports frame stride parameter for extracting every Nth frame

Usage:
------
./bag_to_images <bag_file> <image_topic> [output_dir] [stride]

Parameters:
- bag_file: Path to the ROS bag file
- image_topic: Topic name containing images (e.g., /camera/image_raw/compressed)
- output_dir: (Optional) Directory where images will be saved (default: ./extracted_images)
- stride: (Optional) Extract every Nth frame (default: 1, which means extract all frames)

Example commands:
----------------
# Extract all frames from compressed images:
./bag_to_images recording.bag /camera/image_raw/compressed ~/extracted_images

# Extract every 5th frame from compressed images:
./bag_to_images recording.bag /camera/image_raw/compressed ~/extracted_images 5

# Extract every 10th frame from uncompressed images:
./bag_to_images recording.bag /camera/image_raw ~/extracted_images 10

Dependencies:
------------
- ROS (Robot Operating System)
- OpenCV
- Boost

Building:
--------
cd /home/zzy/bag_tool
./build_and_run.sh


/bag_to_images '/media/zzy/T7/SLAM_DATASET/WHU-HelmetDataset/Heritage_library.bag' /camera0/compressed '/media/zzy/T7/SLAM_DATASET/WHU-HelmetDataset/Heritage_library/images'
