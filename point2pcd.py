#!/usr/bin/env python
import rospy
from sensor_msgs.msg import PointCloud
import pcl
import pcl.pcl_visualization

def callback(msg):
    points = []
    for p in msg.points:
        points.append([p.x, p.y, p.z])
    pc = pcl.PointCloud()
    pc.from_list(points)
    pcl.save(pc, "/home/zzy/pcd/pointcloud.pcd")
    rospy.signal_shutdown("Saved one frame.")

rospy.init_node('pc_save')
rospy.Subscriber('/avia', PointCloud, callback)
rospy.spin()
