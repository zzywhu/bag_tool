#!/usr/bin/env python3

import os
import re
import sys
import argparse
from typing import List, Tuple, Optional

import rospy
import rosbag

from sensor_msgs.msg import PointCloud2

# PCL python bindings are not standard in ROS Noetic on all machines.
# Prefer open3d for reading PCD when available.
try:
    import open3d as o3d
except ImportError:
    o3d = None


_TIMESTAMP_RE = re.compile(r"(-?\d+(?:\.\d+)?)")


def _extract_timestamp_from_name(filename: str) -> Optional[float]:
    """Extract first numeric token from filename as timestamp.

    Examples:
      1636531400.998912096.pcd -> 1636531400.998912096
      00123.pcd -> 123
    """
    base = os.path.basename(filename)
    stem, _ = os.path.splitext(base)
    m = _TIMESTAMP_RE.search(stem)
    if not m:
        return None
    try:
        return float(m.group(1))
    except ValueError:
        return None


def _list_pcd_files_sorted(pcd_dir: str) -> List[Tuple[str, float]]:
    if not os.path.isdir(pcd_dir):
        raise FileNotFoundError(f"PCD directory not found: {pcd_dir}")

    items: List[Tuple[str, float]] = []
    for name in os.listdir(pcd_dir):
        if not name.lower().endswith('.pcd'):
            continue
        path = os.path.join(pcd_dir, name)
        ts = _extract_timestamp_from_name(name)
        if ts is None:
            # Skip files without a timestamp-like token
            continue
        items.append((path, ts))

    items.sort(key=lambda x: x[1])
    return items


def _pcd_to_pointcloud2_open3d(pcd_path: str, frame_id: str) -> PointCloud2:
    if o3d is None:
        raise RuntimeError(
            "open3d is not installed. Install with: pip install open3d\n"
            "(Or provide an alternative PCD reader on your system.)"
        )

    pcd = o3d.io.read_point_cloud(pcd_path)
    pts = pcd.points
    if len(pts) == 0:
        raise ValueError(f"Empty point cloud: {pcd_path}")

    # Build a PointCloud2 with xyz float32
    # Use sensor_msgs.point_cloud2 helper
    import sensor_msgs.point_cloud2 as pc2
    from std_msgs.msg import Header

    header = Header()
    header.frame_id = frame_id

    # open3d returns Vector3dVector (float64). Convert to python tuples.
    points_iter = ((float(p[0]), float(p[1]), float(p[2])) for p in pts)

    fields = [
        pc2.PointField(name='x', offset=0, datatype=pc2.PointField.FLOAT32, count=1),
        pc2.PointField(name='y', offset=4, datatype=pc2.PointField.FLOAT32, count=1),
        pc2.PointField(name='z', offset=8, datatype=pc2.PointField.FLOAT32, count=1),
    ]

    msg = pc2.create_cloud(header, fields, list(points_iter))
    return msg


def pcd_dir_to_bag(
    pcd_dir: str,
    out_bag: str,
    topic: str,
    frame_id: str,
    stamp_mode: str,
    time_offset: float,
):
    files = _list_pcd_files_sorted(pcd_dir)
    if not files:
        raise RuntimeError(f"No .pcd files with parsable timestamps found in: {pcd_dir}")

    os.makedirs(os.path.dirname(os.path.abspath(out_bag)) or '.', exist_ok=True)

    # ROS node is only needed for ros Time helpers; no master required.
    rospy.init_node('pcd2bag', anonymous=True, disable_signals=True)

    with rosbag.Bag(out_bag, 'w') as bag:
        for i, (pcd_path, ts) in enumerate(files, start=1):
            msg = _pcd_to_pointcloud2_open3d(pcd_path, frame_id=frame_id)

            if stamp_mode == 'filename':
                stamp = rospy.Time.from_sec(ts + time_offset)
            elif stamp_mode == 'relative':
                # make first file time=0 (+offset)
                t0 = files[0][1]
                stamp = rospy.Time.from_sec((ts - t0) + time_offset)
            else:
                raise ValueError(f"Unknown stamp_mode: {stamp_mode}")

            msg.header.stamp = stamp

            bag.write(topic, msg, t=stamp)
            if i % 10 == 0:
                print(f"Wrote {i}/{len(files)}: {os.path.basename(pcd_path)} @ {stamp.to_sec():.9f}")

    print(f"Done. Bag written: {out_bag}")


def main():
    parser = argparse.ArgumentParser(
        description='Convert a folder of .pcd files into a ROS bag PointCloud2 topic, sorted by timestamp in filename.'
    )
    parser.add_argument('pcd_dir', help='Directory containing .pcd files')
    parser.add_argument('out_bag', help='Output bag file path (e.g., /path/out.bag)')
    parser.add_argument('--topic', default='/velodyne_points', help='ROS topic to write (default: /velodyne_points)')
    parser.add_argument('--frame-id', default='vlp16', help='frame_id for PointCloud2 (default: vlp16)')
    parser.add_argument(
        '--stamp-mode',
        choices=['filename', 'relative'],
        default='filename',
        help="Use timestamps from filename ('filename') or start from 0 based on first file ('relative').",
    )
    parser.add_argument(
        '--time-offset',
        type=float,
        default=0.0,
        help='Seconds added to computed timestamp (default: 0.0).',
    )

    args = parser.parse_args()

    try:
        pcd_dir_to_bag(
            pcd_dir=args.pcd_dir,
            out_bag=args.out_bag,
            topic=args.topic,
            frame_id=args.frame_id,
            stamp_mode=args.stamp_mode,
            time_offset=args.time_offset,
        )
    except Exception as e:
        print(f"ERROR: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
