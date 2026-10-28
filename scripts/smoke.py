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

# Exercise the installed Qt Image Formats WebP plugin through the same
# composition and FFmpeg worker paths used by the editor.
webp_source = out / "transparency.webp"
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i",
     "nullsrc=size=96x64:rate=1,format=rgba", "-vf",
     "geq=r='0':g='255*gte(X,48)':b='0':a='255*gte(X,48)'",
     "-frames:v", "1", "-c:v", "libwebp", "-lossless", "1", webp_source])
webp_decode = subprocess.run([str(ffmpeg), "-hide_banner", "-loglevel", "error", "-i", str(webp_source),
                              "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgba", "pipe:1"],
                             capture_output=True, timeout=30)
assert webp_decode.returncode == 0, webp_decode.stderr.decode("utf-8", errors="replace")
assert len(webp_decode.stdout) == 96 * 64 * 4
assert webp_decode.stdout[(2 * 96 + 2) * 4 + 3] == 0
assert webp_decode.stdout[(32 * 96 + 70) * 4 + 3] >= 240
webp_project_data = json.loads(project.read_text(encoding="utf-8"))
webp_node = next(node.copy() for node in webp_project_data["nodes"] if node["kind"] == "shape")
webp_node.update({"id": "smoke-webp", "name": "Translucent WebP", "kind": "image", "source": str(webp_source),
                  "parentId": "", "shotId": "", "x": 980, "y": 500, "width": 240, "height": 160,
                  "start": 0, "duration": webp_project_data["duration"], "opacity": 1,
                  "animated": False, "keyframes": [], "captionWords": []})
webp_project_data["nodes"].append(webp_node)
webp_project = out / "webp.studio.json"
webp_project.write_text(json.dumps(webp_project_data, ensure_ascii=False, indent=2), encoding="utf-8")
webp_worker = [bin_dir / "studio_render.exe", "--project", webp_project]
webp_still = out / "webp-expected.png"
webp_video = out / "webp.mp4"
run(webp_worker + ["--still", "90", "--output", webp_still])
webp_render = run(webp_worker + ["--output", webp_video, "--ffmpeg", ffmpeg])
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", webp_video,
     "-vf", "select=eq(n\\,90)", "-frames:v", "1", out / "webp-decoded.png"])
webp_comparison = run([ffmpeg, "-hide_banner", "-i", webp_still, "-i", out / "webp-decoded.png",
                       "-lavfi", "psnr", "-f", "null", "-"])
webp_match = re.search(r"average:([0-9.]+|inf)", webp_comparison.stderr)
assert webp_match, webp_comparison.stderr
webp_psnr = float(webp_match.group(1))
assert webp_psnr > 30, f"WebP preview/export discrepancy: {webp_psnr} dB"
(out / "webp-render.log").write_text(webp_render.stdout + webp_render.stderr, encoding="utf-8")
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
           "durationSeconds": 12, "previewExportPsnrDb": psnr, "webpPreviewExportPsnrDb": webp_psnr,
           "webpAlpha": "transparent and opaque pixels preserved by source decode",
           "videoBytes": video.stat().st_size,
           "elapsedSeconds": round(time.monotonic() - started, 2),
           "checks": ["QML startup", "window capture", "MP4 encoding", "360 decoded frames",
                      "preview/export frame comparison", "WebP alpha decode", "WebP still/video rendering",
                      "WebP preview/export frame comparison", "overwrite refused", "cancellation",
                      "missing media refused"]}
(out / "report.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
print(json.dumps({"artifacts": str(out), **summary}, indent=2))
