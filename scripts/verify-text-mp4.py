import json
from pathlib import Path
import subprocess
import sys

artifact = Path(sys.argv[1]).resolve()
ffmpeg = Path(sys.argv[2]).resolve()
video = artifact / "text-temporal-check.mp4"
if not video.exists():
    raise SystemExit(f"Expected MP4 is missing: {video}")
report = {"status": "passed", "video": str(video), "frames": []}
try:
    from PIL import Image, ImageChops, ImageStat
except ImportError as exc:
    raise SystemExit("Pillow is required by the project's local Python tools") from exc

for index, frame in enumerate((0, 15, 30, 45, 59)):
    decoded = artifact / f"decoded-text-{frame:03}.png"
    subprocess.run([str(ffmpeg), "-hide_banner", "-loglevel", "error", "-i", str(video),
                    "-vf", f"select=eq(n\\,{frame})", "-vsync", "0", "-frames:v", "1", str(decoded)],
                   check=True, capture_output=True)
    reference = Image.open(artifact / f"windows-reference-{frame:03}.png").convert("RGB")
    actual = Image.open(decoded).convert("RGB")
    if actual.size != reference.size:
        raise SystemExit(f"Frame {frame}: unexpected decoded size {actual.size}")
    mae = sum(ImageStat.Stat(ImageChops.difference(reference, actual)).mean) / 3
    title_box = (140, 158, 1140, 258)
    subtitle_box = (140, 266, 1140, 350)
    caption_box = (154, 604, 1126, 672)
    def crop_mae(box):
        delta = ImageChops.difference(reference.crop(box), actual.crop(box))
        return sum(ImageStat.Stat(delta).mean) / 3
    report["frames"].append({"frame": frame, "rgbMae": mae, "titleRgbMae": crop_mae(title_box),
                            "subtitleRgbMae": crop_mae(subtitle_box), "captionRgbMae": crop_mae(caption_box),
                            "size": list(actual.size)})
(artifact / "mp4-comparison.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps(report, indent=2))
