"""Focused UI component slots -> managed project -> QPainter preview -> short MP4 smoke."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

root = Path(sys.argv[1]).resolve()
out = root / "artifacts" / time.strftime("ui-components-smoke-%Y%m%d-%H%M%S")
out.mkdir(parents=True, exist_ok=False)
bin_dir = Path(os.environ.get("STUDIO_BIN_DIR", root / "build/bin"))
ffmpeg = Path(os.environ["STUDIO_FFMPEG"])


def run(args, timeout=120):
    result = subprocess.run([str(arg) for arg in args], capture_output=True, timeout=timeout)
    if result.returncode:
        raise RuntimeError(f"{args[0]} exited {result.returncode}\n"
                           f"{result.stdout.decode(errors='replace')}\n"
                           f"{result.stderr.decode(errors='replace')}")
    return result


image = out / "slot-image.png"
video = out / "slot-video.mp4"
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i",
     "color=c=0xE34859:s=320x640:d=1", "-frames:v", "1", image])
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i",
     "testsrc2=size=480x800:rate=30:duration=3", "-an", "-c:v", "libx264", "-preset", "ultrafast",
     "-pix_fmt", "yuv420p", video])

recipe = {
    "format": "studio-dark-production", "version": 3, "catalogVersion": 1,
    "title": "Teste curto de componentes UI mascarados", "voice": "auto",
    "captions": True, "musicGain": 0, "effectsGain": 0,
    "scenes": [{
        "id": "componentes", "narration": ["Telas, molduras e vídeos continuam editáveis."],
        "shots": [{
            "id": "componentes-shot", "name": "Dispositivos com conteúdo editável",
            "start": {"cue": 0, "cueProgress": 0},
            "camera": {"x": 640, "y": 360, "zoom": 1},
            "elements": [
                {"id": "phone", "kind": "library", "assetId": "ui:device-smartphone",
                 "x": 80, "y": 100,
                 "slots": {"screen": {"type": "image", "source": image.name}}},
                {"id": "tablet", "kind": "library", "assetId": "ui:device-tablet",
                 "x": 540, "y": 70, "scale": 0.76,
                 "slots": {"screen": {"type": "video", "source": video.name}}},
                {"id": "panel", "kind": "library", "assetId": "ui:documentary-panel",
                 "x": 590, "y": 510, "scale": 0.52,
                 "slots": {"body": "MÍDIA EDITÁVEL"}}
            ]
        }]
    }]
}
recipe_path = out / "ui-components.production.json"
recipe_path.write_text(json.dumps(recipe, ensure_ascii=False, indent=2), encoding="utf-8")
workspace = out / "workspace"
production = out / "production"
produce = run([bin_dir / "studio_produce.exe", "--recipe", recipe_path, "--workspace-root", workspace,
               "--output-dir", production], timeout=180)
(out / "production.log").write_bytes(produce.stdout + produce.stderr)
project_path = production / "project.studio.json"
project = json.loads(project_path.read_text(encoding="utf-8"))
report = json.loads((production / "production-report.json").read_text(encoding="utf-8"))
assert report["status"] == "ready", report
assert project["schemaVersion"] == 17, project["schemaVersion"]
masked_images = [node for node in project["nodes"] if node.get("role") == "ui:slot:screen"
                 and node["kind"] == "image" and node.get("mask")]
masked_videos = [node for node in project["nodes"] if node.get("role") == "ui:slot:screen"
                 and node["kind"] == "video" and node.get("mask")]
assert masked_images and masked_videos, "both media types must retain the same component mask"
assert all(Path(node["source"]).is_file() and
           Path(node["source"]).resolve().is_relative_to(workspace.resolve())
           for node in masked_images + masked_videos), "slot media must be copied to managed workspace storage"
assert any(node.get("role") == "ui:slot:body" and node.get("text") == "MÍDIA EDITÁVEL"
           for node in project["nodes"]), "text slots remain ordinary editable text layers"

frame = min(15, project["duration"] - 1)
still = out / "preview.png"
run([bin_dir / "studio_render.exe", "--project", project_path, "--still", frame, "--output", still])
ui = run([bin_dir / "studio_dark.exe", "--smoke", out / "interface", "--project", project_path,
          "--frame", frame], timeout=30)
(out / "desktop.log").write_bytes(ui.stderr)
for marker in ("failed to load", "ReferenceError", "TypeError", "Cannot assign", "Binding loop"):
    assert marker.encode() not in ui.stderr, ui.stderr.decode(errors="replace")
runtime = json.loads((out / "interface" / "runtime.json").read_text(encoding="utf-8"))
assert runtime["page"] == "editor", runtime

movie = out / "ui-components.mp4"
render = run([bin_dir / "studio_render.exe", "--project", project_path, "--output", movie,
              "--ffmpeg", ffmpeg], timeout=120)
(out / "render.log").write_bytes(render.stdout + render.stderr)
decode = run([ffmpeg, "-hide_banner", "-i", movie, "-f", "null", "-"])
assert re.search(rf"frame=\s*{project['duration']}\b", decode.stderr.decode(errors="replace")), \
    decode.stderr.decode(errors="replace")
decoded_frame = out / "export-frame.png"
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", movie,
     "-vf", f"select=eq(n\\,{frame})", "-frames:v", "1", decoded_frame])
comparison = run([ffmpeg, "-hide_banner", "-i", still, "-i", decoded_frame,
                  "-lavfi", "psnr", "-f", "null", "-"])
match = re.search(r"average:([0-9.]+|inf)", comparison.stderr.decode(errors="replace"))
assert match and (match.group(1) == "inf" or float(match.group(1)) > 30), \
    comparison.stderr.decode(errors="replace")

summary = {
    "status": "passed", "components": ["ui:device-smartphone", "ui:device-tablet", "ui:documentary-panel"],
    "projectSchema": project["schemaVersion"], "frameCount": project["duration"], "proofFrame": frame,
    "previewExportPsnrDb": match.group(1), "managedMaskedImageSlots": len(masked_images),
    "managedMaskedVideoSlots": len(masked_videos), "editableTextSlots": 1,
    "checks": ["registered library IDs", "image and video slot media managed by workspace",
               "persisted alpha-mask references", "QPainter still preview", "editor opens project",
               "short MP4 export", "export frame matches QPainter preview"]
}
(out / "report.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
print(json.dumps({"artifacts": str(out), **summary}, ensure_ascii=False, indent=2))
