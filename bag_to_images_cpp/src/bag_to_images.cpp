#include <ros/ros.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/CompressedImage.h>
#include <opencv2/highgui/highgui.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <signal.h>

// Alias for convenience
namespace fs = boost::filesystem;

// Global variables to handle signal interruption
volatile sig_atomic_t g_request_shutdown = 0;
std::mutex g_shutdown_mutex;

// Signal handler for clean shutdown
void sigintHandler(int sig)
{
  std::lock_guard<std::mutex> lock(g_shutdown_mutex);
  g_request_shutdown = 1;
}

void printUsage() {
  std::cout << "Usage: bag_to_images <bag_file> <image_topic> [output_dir] [stride]" << std::endl;
  std::cout << "  <bag_file>: Path to the bag file to extract images from" << std::endl;
  std::cout << "  <image_topic>: Name of the image topic in the bag (supports both raw and compressed)" << std::endl;
  std::cout << "  [output_dir]: Optional directory to save images (default: ./extracted_images)" << std::endl;
  std::cout << "  [stride]: Optional frame stride - save every Nth frame (default: 1, save all frames)" << std::endl;
}

int main(int argc, char** argv) {
  // Initialize ROS with minimal communication
  ros::init(argc, argv, "bag_to_images", ros::init_options::NoSigintHandler);
  
  // Register our own signal handler
  signal(SIGINT, sigintHandler);
  
  // Create local node handle
  ros::NodeHandle nh;

  // Check command line arguments
  if (argc < 3) {
    std::cerr << "Error: Not enough arguments" << std::endl;
    printUsage();
    return 1;
  }

  // Parse command line arguments
  std::string bag_file = argv[1];
  std::string image_topic = argv[2];
  std::string output_dir = (argc > 3) ? argv[3] : "./extracted_images";
  int stride = 1;  // Default: save every frame
  
  if (argc > 4) {
    try {
      stride = std::stoi(argv[4]);
      if (stride < 1) {
        std::cerr << "Warning: Invalid stride value, using default stride of 1" << std::endl;
        stride = 1;
      }
    } catch (const std::exception& e) {
      std::cerr << "Warning: Invalid stride format, using default stride of 1" << std::endl;
      stride = 1;
    }
  }

  try {
    // Verify bag file exists
    if (!fs::exists(bag_file)) {
      std::cerr << "Error: Bag file does not exist: " << bag_file << std::endl;
      return 1;
    }

    // Create output directory if it doesn't exist
    if (!fs::exists(output_dir)) {
      std::cout << "Creating output directory: " << output_dir << std::endl;
      fs::create_directories(output_dir);
    }

    // Open the bag file
    rosbag::Bag bag;
    try {
      std::cout << "Opening bag file: " << bag_file << std::endl;
      bag.open(bag_file, rosbag::bagmode::Read);
    } catch (rosbag::BagException& e) {
      std::cerr << "Error: Could not open bag file: " << e.what() << std::endl;
      return 1;
    }

    // Create a view of messages with the specified topic
    std::vector<std::string> topics;
    topics.push_back(image_topic);
    
    std::cout << "Creating bag view for topic: " << image_topic << std::endl;
    rosbag::View view(bag, rosbag::TopicQuery(topics));
    
    if (view.size() == 0) {
      std::cerr << "Error: No messages found on topic: " << image_topic << std::endl;
      bag.close();
      return 1;
    }

    std::cout << "Processing bag: " << bag_file << std::endl;
    std::cout << "Looking for image topic: " << image_topic << std::endl;
    std::cout << "Saving images to: " << output_dir << std::endl;
    std::cout << "Number of messages: " << view.size() << std::endl;
    std::cout << "Using stride: " << stride << " (saving every " << stride << " frames)" << std::endl;

    // Determine message type by checking the first message
    bool is_compressed = false;
    bool message_type_determined = false;
    
    for (rosbag::View::iterator it = view.begin(); it != view.end(); ++it) {
      if (it == view.begin()) {
        rosbag::MessageInstance const& first_msg = *it;
        std::string datatype = first_msg.getDataType();
        std::cout << "Message datatype: " << datatype << std::endl;
        
        is_compressed = (datatype == "sensor_msgs/CompressedImage");
        message_type_determined = true;
        
        std::cout << "Detected message type: " << (is_compressed ? "CompressedImage" : "Image") << std::endl;
        break;
      }
    }
    
    if (!message_type_determined) {
      std::cerr << "Error: Failed to determine message type" << std::endl;
      bag.close();
      return 1;
    }

    // Counter for processed images
    int count = 0;
    int error_count = 0;
    int frame_count = 0;  // Counter for all frames encountered

    // Process each message
    BOOST_FOREACH(rosbag::MessageInstance const m, view) {
      // Check for shutdown request
      {
        std::lock_guard<std::mutex> lock(g_shutdown_mutex);
        if (g_request_shutdown) {
          std::cout << "\nShutdown requested. Exiting gracefully..." << std::endl;
          break;
        }
      }
      
      // Apply stride - only process every Nth frame
      frame_count++;
      if ((frame_count - 1) % stride != 0) {
        continue;  // Skip this frame based on stride
      }
      
      cv::Mat image;
      ros::Time timestamp;
      bool conversion_ok = false;

      try {
        if (is_compressed) {
          // Handle compressed image
          sensor_msgs::CompressedImage::ConstPtr compressed_img_msg = m.instantiate<sensor_msgs::CompressedImage>();
          
          if (compressed_img_msg != nullptr) {
            // Verify that we have data
            if (compressed_img_msg->data.size() > 0) {
              // Convert compressed image to OpenCV Mat
              cv::Mat compressed_data(1, compressed_img_msg->data.size(), CV_8UC1, const_cast<unsigned char*>(&compressed_img_msg->data[0]));
              image = cv::imdecode(compressed_data, cv::IMREAD_COLOR);
              timestamp = compressed_img_msg->header.stamp;
              conversion_ok = !image.empty();
            } else {
              std::cerr << "Warning: Empty compressed image data" << std::endl;
            }
          } else {
            std::cerr << "Warning: Failed to instantiate CompressedImage message" << std::endl;
          }
        } else {
          // Handle regular image
          sensor_msgs::Image::ConstPtr img_msg = m.instantiate<sensor_msgs::Image>();
          
          if (img_msg != nullptr) {
            // Convert ROS image to OpenCV format with safe encoding
            cv_bridge::CvImageConstPtr cv_ptr;
            try {
              cv_ptr = cv_bridge::toCvShare(img_msg, "bgr8");
            } catch (cv_bridge::Exception&) {
              // If bgr8 fails, try with original encoding
              cv_ptr = cv_bridge::toCvShare(img_msg);
            }
            
            if (cv_ptr && !cv_ptr->image.empty()) {
              image = cv_ptr->image;
              timestamp = img_msg->header.stamp;
              conversion_ok = true;
            }
          } else {
            std::cerr << "Warning: Failed to instantiate Image message" << std::endl;
          }
        }
      } catch (std::exception& e) {
        std::cerr << "Error processing image: " << e.what() << std::endl;
        error_count++;
        continue;
      }
      
      if (conversion_ok && !image.empty()) {
        try {
          // Create filename using ROS timestamp in "seconds.nanoseconds" format
          std::stringstream ss;
          ss << timestamp.sec << "." << std::setfill('0') << std::setw(9) << timestamp.nsec;
          
          std::string filename = output_dir + "/" + ss.str() + ".png";

          // Save the image
          if (!cv::imwrite(filename, image)) {
            std::cerr << "Error: Failed to save image: " << filename << std::endl;
            error_count++;
            continue;
          }

          count++;

          // Print progress every 100 saved images
          if (count % 100 == 0) {
            std::cout << "Processed " << count << " images (examined " << frame_count << " frames)..." << std::endl;
          }
        } catch (std::exception& e) {
          std::cerr << "Error saving image: " << e.what() << std::endl;
          error_count++;
        }
      } else if (conversion_ok) {
        std::cerr << "Warning: Empty image after conversion" << std::endl;
        error_count++;
      }
    }

    // Close the bag file
    bag.close();

    std::cout << "Done! Extracted " << count << " images from " << frame_count << " total frames" << std::endl;
    std::cout << "Stride used: " << stride << " (saved every " << stride << " frames)" << std::endl;
    std::cout << "Images saved to: " << output_dir << std::endl;
    if (error_count > 0) {
      std::cout << "Warning: " << error_count << " images failed during processing" << std::endl;
    }
    
    return 0;
    
  } catch (std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
}
