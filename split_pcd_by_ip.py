#!/usr/bin/env python3

import os
import sys
import argparse
import shutil
from typing import Iterator, Tuple


def ensure_dir(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def classify(filename: str, ip_a: str, ip_b: str) -> str:
    """Return 'A', 'B', or '' based on whether filename stem ends with ip_a/ip_b."""
    base = os.path.basename(filename)
    stem, _ = os.path.splitext(base)
    if stem.endswith(ip_a):
        return 'A'
    if stem.endswith(ip_b):
        return 'B'
    return ''


def iter_files(root: str, recursive: bool) -> Iterator[Tuple[str, str]]:
    if recursive:
        for dirpath, _, filenames in os.walk(root):
            for name in filenames:
                yield dirpath, name
    else:
        for name in os.listdir(root):
            yield root, name


def main():
    parser = argparse.ArgumentParser(
        description="Split files into two folders by filename suffix (e.g., ...192.168.1.200.pcd vs ...192.168.1.201.pcd)."
    )
    parser.add_argument('src_dir', help='Source directory containing files')
    parser.add_argument('out_a', help='Output dir for IP A files')
    parser.add_argument('out_b', help='Output dir for IP B files')
    parser.add_argument('--ip-a', default='192.168.1.200', help='Suffix for group A (default: 192.168.1.200)')
    parser.add_argument('--ip-b', default='192.168.1.201', help='Suffix for group B (default: 192.168.1.201)')
    parser.add_argument('--ext', default='.pcd', help='Only process files with this extension (default: .pcd)')
    parser.add_argument('--copy', action='store_true', help='Copy instead of move')
    parser.add_argument('--recursive', action='store_true', help='Recurse into subfolders')
    parser.add_argument('--dry-run', action='store_true', help='Show actions without changing files')
    args = parser.parse_args()

    if not os.path.isdir(args.src_dir):
        print(f"ERROR: src_dir not found: {args.src_dir}")
        return 1

    ensure_dir(args.out_a)
    ensure_dir(args.out_b)

    moved_a = 0
    moved_b = 0
    skipped = 0

    for dirpath, name in iter_files(args.src_dir, args.recursive):
        src = os.path.join(dirpath, name)
        if not os.path.isfile(src):
            continue
        if args.ext and not name.lower().endswith(args.ext.lower()):
            continue

        group = classify(name, args.ip_a, args.ip_b)
        if group == 'A':
            dst_dir = args.out_a
            moved_a += 1
        elif group == 'B':
            dst_dir = args.out_b
            moved_b += 1
        else:
            skipped += 1
            continue

        dst = os.path.join(dst_dir, name)
        if os.path.abspath(src) == os.path.abspath(dst):
            continue

        action = 'COPY' if args.copy else 'MOVE'
        if args.dry_run:
            print(f"{action}: {src} -> {dst}")
            continue

        if args.copy:
            shutil.copy2(src, dst)
        else:
            shutil.move(src, dst)

    print("Done.")
    print(f"  A ({args.ip_a}) -> {args.out_a}: {moved_a}")
    print(f"  B ({args.ip_b}) -> {args.out_b}: {moved_b}")
    print(f"  Skipped (no suffix match): {skipped}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
