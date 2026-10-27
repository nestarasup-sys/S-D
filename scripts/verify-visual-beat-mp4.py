import argparse
import json
from pathlib import Path

from PIL import Image


def changed_pixels(base, candidate, box, threshold=36):
    a = base.crop(box).convert("RGB")
    b = candidate.crop(box).convert("RGB")
    count = 0
    for left, right in zip(a.getdata(), b.getdata()):
        if sum(abs(x - y) for x, y in zip(left, right)) >= threshold:
            count += 1
    return count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--frames", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    metadata = json.loads(args.report.read_text(encoding="utf-8"))
    first, second, third = metadata["revealFrames"]
    frame_files = sorted(args.frames.glob("frame-*.png"))
    if len(frame_files) <= third:
        raise SystemExit(f"MP4 has {len(frame_files)} decoded frames, expected after frame {third}.")
    frames = [Image.open(path).convert("RGB") for path in frame_files]
    if frames[0].size != (1280, 720):
        raise SystemExit(f"Unexpected video size: {frames[0].size}")
    boxes = [(100, 220, 320, 410), (530, 220, 750, 410), (960, 220, 1180, 410)]
    checks = [
        ("chip-before", first - 1, 0, False),
        ("chip-at", first, 0, True),
        ("network-before", second - 1, 1, False),
        ("network-at", second, 1, True),
        ("brain-before", third - 1, 2, False),
        ("brain-at", third, 2, True),
    ]
    results = []
    for name, frame_index, box_index, expected_visible in checks:
        changed = changed_pixels(frames[first - 1], frames[frame_index], boxes[box_index])
        passed = changed > 500 if expected_visible else changed < 500
        results.append({"check": name, "frame": frame_index, "changedPixels": changed,
                        "expectedVisible": expected_visible, "passed": passed})
    result = {"status": "passed" if all(item["passed"] for item in results) else "failed",
              "decodedFrames": len(frame_files), "resolution": "1280x720", "checks": results}
    args.output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    if result["status"] != "passed":
        raise SystemExit(1)


if __name__ == "__main__":
    main()
