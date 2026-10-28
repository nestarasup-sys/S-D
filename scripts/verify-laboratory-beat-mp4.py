import json
import math
from pathlib import Path
import subprocess
import sys

from PIL import Image

case = Path(sys.argv[1]).resolve()
ffmpeg = Path(sys.argv[2]).resolve()
project = json.loads((case / "production/project.studio.json").read_text(encoding="utf-8"))
movie = case / "laboratorio-completo-stage11.mp4"
targets = ("vb-first", "vb-second", "vb-third")
beats = project["visualBeats"]
nodes = {node["id"]: node for node in project["nodes"]}
selected = []
for target in targets:
    match = next((beat for beat in beats if beat.get("shotId") == "beats-tres-revelacoes" and
                  beat.get("action") == "show" and beat.get("targetId") == target), None)
    if match is None or target not in nodes:
        raise SystemExit(f"Missing reveal beat or target {target}")
    selected.append((target, int(match["frame"]), nodes[target]))

frames = [frame + offset for _, frame, _ in selected for offset in (-1, 0, 1)]
decoded_dir = case / "mp4-keyframes"
decoded_dir.mkdir(parents=True, exist_ok=True)
expression = "+".join(f"eq(n\\,{frame})" for frame in frames)
subprocess.run([str(ffmpeg), "-y", "-hide_banner", "-loglevel", "error", "-i", str(movie),
                "-vf", f"select='{expression}'", "-fps_mode", "passthrough",
                str(decoded_dir / "frame-%02d.png")], check=True, capture_output=True)
images = [Image.open(decoded_dir / f"frame-{index:02d}.png").convert("RGB")
          for index in range(1, len(frames) + 1)]
expected_size = (project["width"], project["height"])
if any(image.size != expected_size for image in images):
    raise SystemExit(f"Unexpected MP4 frame dimensions; expected {expected_size}")

def target_box(node):
    if node.get("parentId"):
        raise SystemExit("This verifier requires the reveal targets to be root scene elements")
    margin = max(30, round(min(node["width"], node["height"]) * 0.12))
    left = max(0, math.floor(node["x"] - margin))
    top = max(0, math.floor(node["y"] - margin))
    right = min(project["width"], math.ceil(node["x"] + node["width"] + margin))
    bottom = min(project["height"], math.ceil(node["y"] + node["height"] + margin))
    return left, top, right, bottom

def changed_pixels(before, after, box):
    a, b = before.crop(box), after.crop(box)
    return sum(sum(abs(c1 - c2) for c1, c2 in zip(x, y)) >= 36
               for x, y in zip(a.getdata(), b.getdata()))

checks = []
for index, (target, frame, node) in enumerate(selected):
    before, event, after = images[index * 3:index * 3 + 3]
    box = target_box(node)
    appeared = changed_pixels(before, event, box)
    persisted = changed_pixels(event, after, box)
    checks.append({"target": target, "frames": [frame - 1, frame, frame + 1],
                   "changedPixelsAtShow": appeared, "changedPixelsAfterShow": persisted,
                   "passed": appeared > 500 and persisted < 500})
report = {"status": "passed" if all(check["passed"] for check in checks) else "failed",
          "sourceRecipe": "STUDIO-DARK-LABORATORIO-DE-MOVIMENTO.production.json",
          "decodedResolution": f"{expected_size[0]}x{expected_size[1]}", "checks": checks}
(case / "laboratory-beat-mp4-check.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps(report, indent=2))
if report["status"] != "passed":
    raise SystemExit(1)
