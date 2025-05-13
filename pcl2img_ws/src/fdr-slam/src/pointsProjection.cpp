#include <ros/ros.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/PointCloud2.h>
#include <nav_msgs/Odometry.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
////////////////////////////////////////////
#include <cmath>
#include <vector>
#include <string>
#include <deque>
////////////////////////////////////////////
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/kdtree/kdtree_flann.h>
/////////////////////////////////////////////
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>

// using std::atan2;
// using std::cos;
// using std::sin;
typedef pcl::PointXYZI PointType;
typedef Eigen::Matrix<double, 6, 1> Vector6d;

ros::Publisher pubLaserCloud;
image_transport::Publisher pubProjectedImage;
// std::vector<ros::Publisher> pubEachScan;
double MINIMUM_RANGE = 0.1;
////////////////////////////////////////////////pcl::PointCloud<PointType> laserCloudIn;
float thta = 0.5; //70degree
std::deque<cv::Mat> imageQuene;
double depthMax=0;

cv::Vec3b FalseColor(int R)
{
    cv::Vec3b color;
    switch (R / 64)
    {
        case 0:
            color = cv::Vec3b(0, 4 * R,255);
            break;
        case 1:
            color = cv::Vec3b(0, 255, 510 - 4 * R);
            break;
        case 2:
            color = cv::Vec3b(4 * R - 510, 255, 0);
            break;
        case 3:
            color = cv::Vec3b(255, 1020 - 4 * R, 0);
            break;
    }
    return color;
}
cv::Mat Proj2Img(cv::Mat InputImage, const int density, pcl::PointCloud<PointType> lidar_cloud)
{
    depthMax = 0;
    std::vector<double> IntrinsicVec({3490.16543639052, 0.0, 1556.80196748468, 0.0, 3476.19182807473, 1122.64725443337, 0.0, 0.0, 1.0});
    std::vector<double>dist_coeffs({-0.0928579725599947, 0.204433911852655, 0.00300851862332235, 0.00254110291866720, -0.0404957345758367});

    float fx_, fy_, cx_, cy_, k1_, k2_, p1_, p2_, k3_, s_;
    fx_ = IntrinsicVec[0];
    cx_ = IntrinsicVec[2];
    fy_ = IntrinsicVec[4];
    cy_ = IntrinsicVec[5];
    k1_ = dist_coeffs[0];
    k2_ = dist_coeffs[1];
    p1_ = dist_coeffs[2];
    p2_ = dist_coeffs[3];
    k3_ = dist_coeffs[4];
    //////////////////////////////////////////////////////////////////////////////
    Eigen::Matrix3d init_rotation_matrix_;
    init_rotation_matrix_<<-0.112675, -0.993632, 0.000670178, -0.023126, 0.00194815 ,-0.999731, 0.993363, -0.112661, -0.0231983;
    Eigen::Vector3d init_translation_vector_;
    init_translation_vector_<<-0.0225109, -0.0660454, -0.0442759;

    Eigen::Vector3d init_euler_angle = init_rotation_matrix_.eulerAngles(2, 1, 0);
    Eigen::Vector3d init_transation = init_translation_vector_;
    
    Vector6d calib_params;
    calib_params << init_euler_angle(0), init_euler_angle(1), init_euler_angle(2), init_transation(0), init_transation(1), init_transation(2);

    std::vector<cv::Point3f> pts_3d;
    for (size_t i = 0; i < lidar_cloud.size(); i += density)
    {
        pcl::PointXYZI point_3d = lidar_cloud.points[i];
        pts_3d.emplace_back(cv::Point3f(point_3d.x, point_3d.y, point_3d.z));
        double depth = point_3d.x*point_3d.x+point_3d.y*point_3d.y+point_3d.z*point_3d.z;
        depth = sqrt(depth);
        if(depthMax<depth)depthMax = depth;
    }
    Eigen::AngleAxisd rotation_vector3;
    rotation_vector3 =
        Eigen::AngleAxisd(calib_params[0], Eigen::Vector3d::UnitZ()) *
        Eigen::AngleAxisd(calib_params[1], Eigen::Vector3d::UnitY()) *
        Eigen::AngleAxisd(calib_params[2], Eigen::Vector3d::UnitX());

    cv::Mat CameraMatrix =
        (cv::Mat_<double>(3, 3) << fx_, 0.0, cx_, 0.0, fy_, cy_, 0.0, 0.0, 1.0);
    cv::Mat distortion_coeff =
        (cv::Mat_<double>(1, 5) << k1_, k2_, p1_, p2_, k3_);
    cv::Mat r_vec =
        (cv::Mat_<double>(3, 1)
        << rotation_vector3.angle() * rotation_vector3.axis().transpose()[0],
            rotation_vector3.angle() * rotation_vector3.axis().transpose()[1],
            rotation_vector3.angle() * rotation_vector3.axis().transpose()[2]);
    cv::Mat t_vec = (cv::Mat_<double>(3, 1) << calib_params[3], calib_params[4], calib_params[5]);
    // project 3d-points into image view
    std::vector<cv::Point2f> pts_2d;
    cv::projectPoints(pts_3d, r_vec, t_vec, CameraMatrix, distortion_coeff, pts_2d);
    cv::Mat ImageProjected = InputImage;
    int ImageHeight = ImageProjected.rows;
    int ImageWidth = ImageProjected.cols;

    for (size_t i = 0; i < pts_2d.size(); ++i)
    {
        cv::Point2f point_2d = pts_2d[i];
        double depth = pts_3d[i].x*pts_3d[i].x+pts_3d[i].y*pts_3d[i].y+pts_3d[i].z*pts_3d[i].z;
        depth = sqrt(depth);
        cv::Vec3b Color = FalseColor((uchar)(255*depth/depthMax));//depthMax
        if (point_2d.x >= 1 && point_2d.x < ImageWidth - 1 && point_2d.y >= 1 && point_2d.y < ImageHeight-1)
        {
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y, (int)pts_2d[i].x) = Color;
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y-1, (int)pts_2d[i].x) = Color;
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y, (int)pts_2d[i].x-1) = Color;
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y-1, (int)pts_2d[i].x-1) = Color;
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y+1, (int)pts_2d[i].x) = Color;
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y, (int)pts_2d[i].x+1) = Color;
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y+1, (int)pts_2d[i].x+1) = Color;
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y+1, (int)pts_2d[i].x-1) = Color;
            ImageProjected.at<cv::Vec3b>((int)pts_2d[i].y-1, (int)pts_2d[i].x+1) = Color;
        } 
    }
    return ImageProjected;
}


template <typename PointT> void removeClosedPointCloud(const pcl::PointCloud<PointT> &cloud_in, pcl::PointCloud<PointT> &cloud_out, float thres)
{
    if (&cloud_in != &cloud_out)
    {
        cloud_out.header = cloud_in.header;
        cloud_out.points.resize(cloud_in.points.size());
    }
    size_t j = 0;//long unsigned int //4,294,967,295
    for (size_t i = 0; i < cloud_in.points.size(); ++i)
    {
        float normSqu = cloud_in.points[i].x * cloud_in.points[i].x + cloud_in.points[i].y * cloud_in.points[i].y + cloud_in.points[i].z * cloud_in.points[i].z;
        float cosAngel = cloud_in.points[i].x / sqrt(normSqu);

        // std::cerr <<cosAngel<<std::endl;

        if (normSqu < thres * thres)
            continue;
        if (cosAngel < thta)
            continue;
        cloud_out.points[j] = cloud_in.points[i];
        j++;
    }
    if (j != cloud_in.points.size())
    {
        cloud_out.points.resize(j);
    }
    //不算是直通滤波
    cloud_out.height = 1;
    cloud_out.width = static_cast<uint32_t>(j);
    cloud_out.is_dense = true;
}

void imageCallback(const sensor_msgs::ImageConstPtr& msg)
{
    // std::cerr <<"image in"<<std::endl;
    try   // 如果转换失败，则提跳转到catch语句
    {
        cv::Mat image;
        image = cv_bridge::toCvShare(msg, "bgr8")->image;
        //cv::imwrite("/home/zzy/bag_tool/pcl2img_ws/src/1.png", image);
        imageQuene.push_front(image);
        // imageQuene.clear();
        // exit(0);
    }
    catch(cv_bridge::Exception& e)
    {
        ROS_ERROR("Could not convert for '%s' to 'bgr8'.", msg->encoding.c_str());
    }
}

void laserCloudHandler(const sensor_msgs::PointCloud2ConstPtr &laserCloudMsg)
{
    // std::cerr <<"pc out"<<std::endl;

    // pcl::PointCloud<PointType> laserCloudIn;
    // pcl::fromROSMsg(*laserCloudMsg, laserCloudIn);//ros点云转pcl::PointXYZ类型
    // std::vector<int> indices;

    // pcl::removeNaNFromPointCloud(laserCloudIn, laserCloudIn, indices);
    // removeClosedPointCloud(laserCloudIn, laserCloudIn, MINIMUM_RANGE);//MINIMUM_RANGE=0.1//10cm//去除离圆心过于近的点

    // sensor_msgs::PointCloud2 laserCloudOutMsg;
    // pcl::toROSMsg(laserCloudIn, laserCloudOutMsg);
    // laserCloudOutMsg.header.stamp = laserCloudMsg->header.stamp;
    // laserCloudOutMsg.header.frame_id = "camera_init";
    // pubLaserCloud.publish(laserCloudOutMsg);
    if(imageQuene.size()>1)
    {
        // std::cerr <<"pc in"<<std::endl;
        cv::Mat image = imageQuene.front();

        pcl::PointCloud<PointType> laserCloudIn;
        pcl::fromROSMsg(*laserCloudMsg, laserCloudIn);//ros点云转pcl::PointXYZ类型
        std::vector<int> indices;

        pcl::removeNaNFromPointCloud(laserCloudIn, laserCloudIn, indices);
        removeClosedPointCloud(laserCloudIn, laserCloudIn, MINIMUM_RANGE);//MINIMUM_RANGE=0.1//10cm//去除离圆心过于近的点

        cv::Mat projectedImage;
        projectedImage = Proj2Img(image, 1, laserCloudIn);

        // cv::Mat projectedImage = image;
        cv::imwrite("/home/zzy/bag_tool/pcl2img_ws/src/1.png", projectedImage);
        sensor_msgs::PointCloud2 laserCloudOutMsg;
        pcl::toROSMsg(laserCloudIn, laserCloudOutMsg);
        laserCloudOutMsg.header.stamp = laserCloudMsg->header.stamp;
        laserCloudOutMsg.header.frame_id = "camera_init";
        pubLaserCloud.publish(laserCloudOutMsg);

        sensor_msgs::ImagePtr prjImageMsg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", projectedImage).toImageMsg();
        prjImageMsg->header.stamp = laserCloudMsg->header.stamp;
        prjImageMsg->header.frame_id = "camera_init";
        pubProjectedImage.publish(prjImageMsg);
        
        imageQuene.clear();
    }

}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "pointsProjection");
    ros::NodeHandle nh;
    image_transport::ImageTransport it(nh); // 注册句柄

    nh.param<double>("minimum_range", MINIMUM_RANGE, 0.1);

    ros::Subscriber subLaserCloud = nh.subscribe<sensor_msgs::PointCloud2>("/velodyne_points", 1000, laserCloudHandler);
    image_transport::Subscriber imageSub = it.subscribe("/galaxy_camera/galaxy_camera/image_raw", 1000, imageCallback);  // 订阅/cameraImage话题，并添加回调函数

    pubLaserCloud = nh.advertise<sensor_msgs::PointCloud2>("/mypoints", 1000);
    pubProjectedImage = it.advertise("/projimages", 1000);

    //此处阻塞主函数，等待消息处理函数的调用
    ros::spin();

    return 0;
}
