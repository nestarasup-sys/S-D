"""Exercises native recipe -> Windows speech -> editable project -> real audiovisual MP4."""
import array
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import wave

root = Path(sys.argv[1]).resolve()
folder = root / "artifacts" / time.strftime("production-smoke-%Y%m%d-%H%M%S")
folder.mkdir(parents=True, exist_ok=False)
output = folder / "production"
bin_dir = Path(os.environ.get("STUDIO_BIN_DIR", root / "build/bin"))
ffmpeg = os.environ["STUDIO_FFMPEG"]


def run(args, expected=0, binary=False, timeout=120):
    result = subprocess.run([str(a) for a in args], capture_output=True, timeout=timeout)
    if result.returncode != expected:
        raise RuntimeError(f"{args[0]}: {result.returncode}\n{result.stdout.decode(errors='replace')}\n{result.stderr.decode(errors='replace')}")
    if not binary:
        result.stdout = result.stdout.decode("utf-8", errors="replace")
        result.stderr = result.stderr.decode("utf-8", errors="replace")
    return result


started = time.monotonic()
produce = run([bin_dir / "studio_produce.exe", "--recipe", root / "examples/carros-caros.production.json", "--output-dir", output])
(folder / "production.log").write_text(produce.stdout + produce.stderr, encoding="utf-8")
project_path = output / "project.studio.json"
project = json.loads(project_path.read_text(encoding="utf-8"))
report = json.loads((output / "production-report.json").read_text(encoding="utf-8"))
assert report["status"] == "ready" and len(project["scenes"]) == 6
assert project["schemaVersion"] == 3 and report["frameCount"] == project["duration"]
with wave.open(str(output / "media/master.wav"), "rb") as audio:
    assert audio.getframerate() == 24000 and audio.getnchannels() == 1
    samples = array.array("h", audio.readframes(audio.getnframes()))
    assert len(samples) == project["duration"] * 800
    rms = math.sqrt(sum((x / 32768) ** 2 for x in samples) / len(samples))
    assert rms > 0.01 and max(map(abs, samples)) < 32767
video = output / "carros-caros.mp4"
render = run([bin_dir / "studio_render.exe", "--project", project_path, "--output", video, "--ffmpeg", ffmpeg])
assert json.loads(render.stdout.strip().splitlines()[-1])["progress"] == 100
decode = run([ffmpeg, "-hide_banner", "-i", video, "-f", "null", "-"])
assert "Audio: aac" in decode.stderr and "1280x720" in decode.stderr
assert re.search(rf"frame=\s*{project['duration']}\b", decode.stderr)
decoded_audio = run([ffmpeg, "-hide_banner", "-loglevel", "error", "-i", video, "-map", "0:a:0", "-f", "s16le", "-ac", "1", "-ar", "24000", "pipe:1"], binary=True)
assert abs(len(decoded_audio.stdout) // 2 - len(samples)) < 2048
frames = [s["start"] + min(60, s["duration"] - 1) for s in project["scenes"]]
for index, frame in enumerate(frames, 1):
    run([bin_dir / "studio_render.exe", "--project", project_path, "--still", frame, "--output", output / f"shot-{index}.png"])
select = "+".join(f"eq(n\\,{frame})" for frame in frames)
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", video, "-vf", f"select={select}", "-fps_mode", "vfr", output / "decoded-%d.png"])
comparison = run([ffmpeg, "-hide_banner", "-i", output / "shot-%d.png", "-i", output / "decoded-%d.png", "-lavfi", "psnr", "-f", "null", "-"])
match = re.search(r"average:([0-9.]+|inf)", comparison.stderr)
assert match and float(match.group(1)) > 30
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", output / "shot-%d.png", "-vf", "scale=640:360,tile=2x3", "-frames:v", "1", output / "storyboard.png"])
ui = run([bin_dir / "studio_dark.exe", "--smoke", output / "interface", "--project", project_path, "--frame", 90, "--play-smoke"], timeout=30)
(folder / "desktop.log").write_text(ui.stderr, encoding="utf-8")
for marker in ("failed to load", "ReferenceError", "TypeError", "Cannot assign", "Binding loop"):
    assert marker not in ui.stderr, ui.stderr
runtime = json.loads((output / "interface/runtime.json").read_text(encoding="utf-8"))
assert runtime["hasAudio"] and runtime["playing"] and runtime["frame"] > 90, (runtime, ui.stderr)
# No guessed voices and no partially assembled project presented as complete.
recipe = json.loads((root / "examples/carros-caros.production.json").read_text(encoding="utf-8"))
recipe["voice"] = "__studio_missing_voice__"
bad = folder / "missing-voice.production.json"
bad.write_text(json.dumps(recipe, ensure_ascii=False), encoding="utf-8")
run([bin_dir / "studio_produce.exe", "--recipe", bad, "--output-dir", folder / "missing-voice"], expected=1)
assert not (folder / "missing-voice/project.studio.json").exists()
capabilities_path = folder / "capabilities.json"
run([bin_dir / "studio_produce.exe", "--capabilities", capabilities_path])
catalog = json.loads(capabilities_path.read_text(encoding="utf-8"))
guide = folder / "guide.md"
run([bin_dir / "studio_produce.exe", "--guide", guide])
guide_text = guide.read_text(encoding="utf-8")
assert all(f"`{item['id']}`" in guide_text for item in catalog["nativeAssets"])
# A contact sheet of the entire native library, generated by the actual composition engine.
sheet = dict(project, name="Biblioteca nativa", duration=360, width=1280, height=1080, scenes=[], audioSource="", nodes=[])
base = next(n for n in project["nodes"] if n["kind"] == "native")
for index, item in enumerate(catalog["nativeAssets"]):
    ratio = min(280 / item["width"], 155 / item["height"])
    x, y = 20 + (index % 4) * 320, 35 + (index // 4) * 260
    sheet["nodes"].append(dict(base, id=f"asset-{index}", nativeId=item["id"], sceneId="", role="", name=item["name"], x=x, y=y, width=item["width"] * ratio, height=item["height"] * ratio, start=0, duration=360, animated=False))
    label = dict(base, id=f"label-{index}", kind="text", nativeId="", sceneId="", role="", name=item["name"], text=item["name"], x=x, y=y+180, width=280, height=40, fontSize=20, start=0, duration=360, animated=False)
    sheet["nodes"].append(label)
sheet_path = folder / "library.studio.json"
sheet_path.write_text(json.dumps(sheet, ensure_ascii=False), encoding="utf-8")
run([bin_dir / "studio_render.exe", "--project", sheet_path, "--still", 60, "--output", folder / "library.png"])
# End-to-end fixed timing using the real local SAPI voice (no guessed word boundaries).
timed_output = folder / "fixed-timing"
timed_recipe = root / "examples/tempo-fixo-v2.production.json"
timed_run = run([bin_dir / "studio_produce.exe", "--recipe", timed_recipe,
                 "--output-dir", timed_output], timeout=360)
(folder / "fixed-timing.log").write_text(timed_run.stdout + timed_run.stderr, encoding="utf-8")
timed_project = json.loads((timed_output / "project.studio.json").read_text(encoding="utf-8"))
timed_report = json.loads((timed_output / "production-report.json").read_text(encoding="utf-8"))
assert timed_project["duration"] == 240 and [s["duration"] for s in timed_project["scenes"]] == [120, 120]
assert timed_report["timingMode"] == "fit-to-duration" and timed_report["frameCount"] == 240
assert [v["durationFrames"] for v in timed_report["timingPlan"]] == [120, 120]
assert all(v["voiceSpeed"] <= 1.15 for scene in timed_report["timingPlan"] for v in scene["cues"])
with wave.open(str(timed_output / "media/master.wav"), "rb") as timed_audio:
    assert timed_audio.getnframes() == 240 * 800 and timed_audio.getframerate() == 24000
assert any(k["frame"] > 0 and k["frame"] < 120 for n in timed_project["nodes"]
           for k in n.get("keyframes", [])), "Proportional keyframes were not compiled"
fixed_video = timed_output / "fixed-timing.mp4"
run([bin_dir / "studio_render.exe", "--project", timed_output / "project.studio.json",
     "--output", fixed_video, "--ffmpeg", ffmpeg], timeout=180)
decoded = run([ffmpeg, "-hide_banner", "-i", fixed_video, "-f", "null", "-"])
assert re.search(r"frame=\s*240\b", decoded.stderr), "MP4 must encode exactly 240 video frames"

summary = {"status": "passed", "voice": report["voice"], "scenes": 6, "nodes": len(project["nodes"]), "frames": project["duration"], "durationSeconds": project["duration"] / 30, "audioRms": rms, "previewExportPsnrDb": float(match.group(1)), "elapsedSeconds": round(time.monotonic() - started, 2), "video": str(video), "checks": ["native recipe compiler", "real Windows pt-BR speech", "measured audio timing", "PCM mix", "H264 and AAC decode", "six frame comparisons", "QML window capture", "missing voice rejection", "guide/catalogue consistency", "native library contact sheet", "8-second frame-exact production", "cueProgress after TTS", "240-frame MP4"]}
(folder / "verification.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
print(json.dumps({"artifacts": str(folder), **summary}, indent=2))
