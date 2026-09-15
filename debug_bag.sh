#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BAG_FILE="$1"
TOPIC="$2"

if [ -z "$BAG_FILE" ] || [ -z "$TOPIC" ]; then
  echo "Usage: $0 <bag_file> <image_topic>"
  exit 1
fi

echo "Checking bag file integrity..."
if ! rosbag check "$BAG_FILE"; then
  echo "Warning: Bag file may be corrupted."
fi

echo ""
echo "Checking message count on topic $TOPIC:"
rosbag info "$BAG_FILE" | grep -A 1 "$TOPIC"

echo ""
echo "Checking message type:"
TOPIC_TYPE=$(rostopic type -b "$BAG_FILE" "$TOPIC" 2>/dev/null)
echo "Type: $TOPIC_TYPE"

echo ""
echo "Running with gdb for debug information..."

# Run with gdb to catch segmentation faults
gdb -ex "run $BAG_FILE $TOPIC ./debug_images" -ex "bt" -ex "quit" --args $SCRIPT_DIR/bag_to_images

echo ""
echo "Debug session complete."
