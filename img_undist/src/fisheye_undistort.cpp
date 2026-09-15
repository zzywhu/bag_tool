#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace fs = std::filesystem;

void undistortImage(const cv::Mat& src, cv::Mat& dst, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, const cv::Mat& newCameraMatrix = cv::Mat()) {
    cv::Mat map1, map2;
    cv::Size imageSize = src.size();
    
    // Use new camera matrix if provided, otherwise use the original camera matrix
    cv::Mat outputCameraMatrix = newCameraMatrix.empty() ? cameraMatrix : newCameraMatrix;
    
    // Create undistortion maps
    cv::initUndistortRectifyMap(cameraMatrix, distCoeffs, cv::Mat(), 
                               outputCameraMatrix, imageSize, CV_32FC1, map1, map2);
    
    // Apply undistortion
    cv::remap(src, dst, map1, map2, cv::INTER_LINEAR);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_folder> <output_folder>" << std::endl;
        return -1;
    }

    std::string inputFolder = argv[1];
    std::string outputFolder = argv[2];
    std::string configFile = "/home/zzy/bag_tool/img_undist/config/camera_intrinsics.yaml";

    // Check if input folder exists
    if (!fs::exists(inputFolder)) {
        std::cerr << "Input folder does not exist: " << inputFolder << std::endl;
        return -1;
    }

    // Create output folder if it doesn't exist
    if (!fs::exists(outputFolder)) {
        std::cout << "Creating output folder: " << outputFolder << std::endl;
        fs::create_directories(outputFolder);
    }

    // Load camera parameters from YAML file
    cv::FileStorage fs(configFile, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "Failed to open config file: " << configFile << std::endl;
        return -1;
    }

    cv::Mat cameraMatrix, distCoeffs, newCameraMatrix;
    fs["camera_matrix"] >> cameraMatrix;
    fs["distortion_coefficients"] >> distCoeffs;
    
    // 读取新的内参矩阵，如果存在的话
    fs["new_camera_matrix"] >> newCameraMatrix;
    
    fs.release();

    std::cout << "原始相机内参矩阵: " << std::endl << cameraMatrix << std::endl;
    std::cout << "畸变系数: " << std::endl << distCoeffs << std::endl;
    
    if (!newCameraMatrix.empty()) {
        std::cout << "新的内参矩阵: " << std::endl << newCameraMatrix << std::endl;
    } else {
        std::cout << "没有提供新的内参矩阵，将使用原始内参矩阵" << std::endl;
    }

    // Process all images in the input folder
    int processedCount = 0;
    for (const auto& entry : fs::directory_iterator(inputFolder)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
            // Convert to lowercase for case-insensitive comparison
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            // Check if the file is an image
            if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || 
                extension == ".bmp" || extension == ".tiff" || extension == ".tif") {
                
                std::string inputPath = entry.path().string();
                std::string filename = entry.path().filename().string();
                std::string outputPath = outputFolder + "/" + filename;
                
                cv::Mat image = cv::imread(inputPath);
                if (image.empty()) {
                    std::cerr << "Failed to read image: " << inputPath << std::endl;
                    continue;
                }
                
                cv::Mat undistortedImage;
                undistortImage(image, undistortedImage, cameraMatrix, distCoeffs, newCameraMatrix);
                
                bool success = cv::imwrite(outputPath, undistortedImage);
                if (success) {
                    processedCount++;
                    std::cout << "Processed: " << filename << std::endl;
                } else {
                    std::cerr << "Failed to save image: " << outputPath << std::endl;
                }
            }
        }
    }
    
    std::cout << "Processed " << processedCount << " images from " << inputFolder 
              << " to " << outputFolder << std::endl;
    
    return 0;
}
