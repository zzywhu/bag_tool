# bag_tool

topic2pcd: rosrun pcl_ros pointcloud_to_pcd input:=/livox/lidar

topic2img: rosrun image_view extract_images image:=/blackflys/image_raw

video2gif: python3 video2gif.py '/home/zzy/bag_tool/supp1-3387128.mp4' --start 8 --end 20 --fps 12 --width 480 -o out.gif

pcd2bag: python3 pcd2bag.py /path/to/pcd_dir /path/to/output.bag --topic /velodyne_points --frame-id lidar --stamp-mode filename

split_pcd_by_ip: python3 split_pcd_by_ip.py /path/to/src_dir /path/to/out_200 /path/to/out_201 --ip-a 192.168.1.200 --ip-b 192.168.1.201
