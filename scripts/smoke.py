"""Desktop + renderer + decoder smoke; outputs are disposable artifacts."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

root = Path(sys.argv[1]).resolve()
out = root / "artifacts" / time.strftime("smoke-%Y%m%d-%H%M%S")
out.mkdir(parents=True, exist_ok=False)
bin_dir = Path(os.environ.get("STUDIO_BIN_DIR", root / "build/bin"))
ffmpeg = os.environ["STUDIO_FFMPEG"]


def run(args, expected=0, timeout=120):
    completed = subprocess.run([str(a) for a in args], capture_output=True, text=True,
                               encoding="utf-8", errors="replace", timeout=timeout)
    if completed.returncode != expected:
        raise RuntimeError(f"{args[0]} exited {completed.returncode}\n{completed.stdout}\n{completed.stderr}")
    return completed


started = time.monotonic()
ui = run([bin_dir / "studio_dark.exe", "--smoke", out], timeout=30)
(out / "desktop.log").write_text(ui.stderr, encoding="utf-8")
if any(marker in ui.stderr for marker in ("failed to load", "ReferenceError", "TypeError", "Cannot assign", "Binding loop")):
    raise RuntimeError(f"QML error: {ui.stderr}")
project = out / "demo.studio.json"
video = out / "demo.mp4"
worker = [bin_dir / "studio_render.exe", "--project", project]
render = run(worker + ["--output", video, "--ffmpeg", ffmpeg])
progress = [json.loads(line) for line in render.stdout.splitlines() if line.startswith("{")]
assert progress[-1]["progress"] == 100
assert video.stat().st_size > 10000
(out / "render.log").write_text(render.stdout + render.stderr, encoding="utf-8")
decode = run([ffmpeg, "-hide_banner", "-i", video, "-f", "null", "-"])
assert "1280x720" in decode.stderr and "30 fps" in decode.stderr
assert re.search(r"frame=\s*360\b", decode.stderr), decode.stderr
run(worker + ["--still", "90", "--output", out / "expected.png"])
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", video,
     "-vf", "select=eq(n\\,90)", "-frames:v", "1", out / "decoded.png"])
comparison = run([ffmpeg, "-hide_banner", "-i", out / "expected.png", "-i", out / "decoded.png",
                  "-lavfi", "psnr", "-f", "null", "-"])
match = re.search(r"average:([0-9.]+|inf)", comparison.stderr)
assert match, comparison.stderr
psnr = float(match.group(1))
assert psnr > 30, f"Preview/export discrepancy: {psnr} dB"
before = hashlib.sha256(video.read_bytes()).hexdigest()
run(worker + ["--output", video, "--ffmpeg", ffmpeg], expected=1)
assert hashlib.sha256(video.read_bytes()).hexdigest() == before
cancel = out / "cancel"
cancel.write_text("cancel", encoding="utf-8")
run(worker + ["--output", out / "cancelled.mp4", "--ffmpeg", ffmpeg, "--cancel-file", cancel], expected=130)
assert not (out / "cancelled.mp4").exists()
missing = json.loads(project.read_text(encoding="utf-8"))
missing["nodes"][0]["kind"] = "image"
missing["nodes"][0]["source"] = str(out / "does-not-exist.png")
missing_path = out / "missing.studio.json"
missing_path.write_text(json.dumps(missing), encoding="utf-8")
run([bin_dir / "studio_render.exe", "--project", missing_path, "--output", out / "missing.mp4", "--ffmpeg", ffmpeg], expected=1)
assert not (out / "missing.mp4").exists()
summary = {"status": "passed", "frames": 360, "resolution": "1280x720", "fps": 30,
           "durationSeconds": 12, "previewExportPsnrDb": psnr, "videoBytes": video.stat().st_size,
           "elapsedSeconds": round(time.monotonic() - started, 2),
           "checks": ["QML startup", "window capture", "MP4 encoding", "360 decoded frames",
                      "preview/export frame comparison", "overwrite refused", "cancellation", "missing media refused"]}
(out / "report.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
print(json.dumps({"artifacts": str(out), **summary}, indent=2))
