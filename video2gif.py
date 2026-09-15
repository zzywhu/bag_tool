#!/usr/bin/env python3

import os
import sys
import argparse
from typing import Optional

# We rely on moviepy for robust video reading and GIF writing
try:
    from moviepy.editor import VideoFileClip
    from moviepy.video.fx.all import speedx
except ImportError:
    print("ERROR: moviepy is not installed. Install with:")
    print("pip install moviepy imageio-ffmpeg")
    sys.exit(1)


def parse_time_str(s: Optional[str]):
    """
    Parse a time string into seconds (float).

    Supports formats:
      - seconds as float or int (e.g., "12", "2.5")
      - HH:MM:SS or MM:SS
        (seconds can be fractional)
        e.g., "01:02:03.5", "02:10"

    Returns None if s is None or empty.
    """

    if s is None:
        return None

    s = str(s).strip()

    if not s:
        return None

    if ':' not in s:
        return float(s)

    parts = s.split(':')

    if len(parts) > 3:
        raise ValueError(f"Invalid time format: {s}")

    secs = float(parts[-1])

    # minutes
    if len(parts) >= 2:
        secs += 60 * int(parts[-2])

    # hours
    if len(parts) == 3:
        secs += 3600 * int(parts[-3])

    return secs


def derive_output_path(input_path: str,
                       explicit_output: Optional[str]) -> str:

    if explicit_output:
        out = explicit_output
    else:
        base, _ = os.path.splitext(os.path.basename(input_path))
        out = os.path.join(os.getcwd(), base + ".gif")

    # Ensure parent dir exists
    out_dir = os.path.dirname(os.path.abspath(out))

    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir, exist_ok=True)

    return out


def video_to_gif(
        input_path: str,
        output_path: Optional[str],
        start: Optional[str],
        end: Optional[str],
        fps: Optional[float],
        width: Optional[int],
        loop: int,
        quality: int = 90,
        speed: float = 1.0):

    if not os.path.isfile(input_path):
        print(f"ERROR: Input video not found: {input_path}")
        sys.exit(1)

    start_s = parse_time_str(start)
    end_s = parse_time_str(end)

    clip = None
    sub = None

    try:
        clip = VideoFileClip(input_path)

        duration = float(clip.duration)

        if start_s is not None and (start_s < 0 or start_s >= duration):
            print(
                f"ERROR: --start ({start_s}s) "
                f"must be within [0, {duration:.3f}] seconds"
            )
            sys.exit(1)

        if end_s is not None and (end_s <= 0 or end_s > duration):
            print(
                f"ERROR: --end ({end_s}s) "
                f"must be within (0, {duration:.3f}] seconds"
            )
            sys.exit(1)

        if (start_s is not None and
                end_s is not None and
                end_s <= start_s):
            print("ERROR: --end must be greater than --start")
            sys.exit(1)

        # Apply subclip window
        if start_s is not None or end_s is not None:
            sub = clip.subclip(
                start_s or 0,
                end_s or duration
            )
            target = sub
        else:
            target = clip

        # Resize if requested
        if width is not None:

            if width <= 0:
                print("ERROR: --width must be a positive integer")
                sys.exit(1)

            target = target.resize(width=width)

        # Apply playback speed
        if speed <= 0:
            print("ERROR: --speed must be > 0")
            sys.exit(1)

        if speed != 1.0:
            target = target.fx(speedx, speed)

        # Determine fps for GIF
        eff_fps = fps if fps is not None else (
            getattr(target, 'fps', None) or 15
        )

        if eff_fps <= 0:
            eff_fps = 15

        out = derive_output_path(input_path, output_path)

        print(
            f"Writing GIF with "
            f"fps={eff_fps}, "
            f"quality={quality}%, "
            f"speed={speed}x ..."
        )

        # Write GIF
        target.write_gif(
            out,
            fps=eff_fps,
            loop=loop
        )

        print(f"Saved GIF: {out}")

    finally:

        # Clean-up
        try:
            if sub is not None:
                sub.close()
        except Exception:
            pass

        try:
            if clip is not None:
                clip.close()
        except Exception:
            pass


def main():

    parser = argparse.ArgumentParser(
        description=(
            "Convert a video to GIF "
            "with optional trimming and speed control."
        ),
        epilog=
        "Examples:\n"
        "  video2gif.py input.mp4 -o out.gif\n"
        "  video2gif.py input.mp4 --start 12 --end 20\n"
        "  video2gif.py input.mp4 --speed 2.0\n"
        "  video2gif.py input.mp4 --speed 0.5\n"
        "  video2gif.py input.mp4 "
        "--start 00:01:00 --end 00:01:10 "
        "--fps 12 --width 480 --speed 1.5\n",
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument(
        'input',
        help='Path to the input video file'
    )

    parser.add_argument(
        '-o',
        '--output',
        help='Output GIF path '
             '(default: <cwd>/<video_basename>.gif)'
    )

    parser.add_argument(
        '--start',
        help='Start time '
             '(seconds or HH:MM:SS[.ms])'
    )

    parser.add_argument(
        '--end',
        help='End time '
             '(seconds or HH:MM:SS[.ms])'
    )

    parser.add_argument(
        '--fps',
        type=float,
        help='Output GIF frame rate '
             '(default: from video or 15)'
    )

    parser.add_argument(
        '--width',
        type=int,
        help='Resize GIF to this width '
             'while keeping aspect ratio'
    )

    parser.add_argument(
        '--loop',
        type=int,
        default=0,
        help='Loop count '
             '(0=infinite, 1=play once, etc.)'
    )

    parser.add_argument(
        '--quality',
        type=int,
        default=90,
        help='GIF quality 0-100 '
             '(default: 90)'
    )

    parser.add_argument(
        '--speed',
        type=float,
        default=1.0,
        help='Playback speed multiplier '
             '(default: 1.0, '
             '2.0=faster, '
             '0.5=slower)'
    )

    args = parser.parse_args()

    video_to_gif(
        input_path=args.input,
        output_path=args.output,
        start=args.start,
        end=args.end,
        fps=args.fps,
        width=args.width,
        loop=args.loop,
        quality=args.quality,
        speed=args.speed,
    )


if __name__ == '__main__':
    main()

