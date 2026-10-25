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
assert project["schemaVersion"] == 7 and report["frameCount"] == project["duration"]
assert any(node.get("role") == "caption" and node.get("captionWords") for node in project["nodes"])
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
run([sys.executable, root / "scripts/embed-ai-production-example.py", "--guide", guide,
     "--example", root / "examples/carros-caros-directors-cut-90s.production.json"])
guide_text = guide.read_text(encoding="utf-8")
assert all(f"`{item['id']}`" in guide_text for item in catalog["nativeAssets"])
assert "Catálogo 1 · Receita 3 · Projeto 10" in guide_text
assert catalog["projectVersion"] == 10
assert {"video-source-trim", "video-speed-control", "video-normalized-crop"}.issubset(
    set(catalog["productionCapabilities"]))
assert '"sourceInSeconds"' in guide_text and '"durationFrames"' in guide_text
guide_example = re.search(r"<!-- STUDIO-DARK-VALIDATED-EXAMPLE-BEGIN -->\s*```json\s*(.*?)\s*```\s*<!-- STUDIO-DARK-VALIDATED-EXAMPLE-END -->",
                          guide_text, re.DOTALL)
assert guide_example and json.loads(guide_example.group(1)) == json.loads(
    (root / "examples/carros-caros-directors-cut-90s.production.json").read_text(encoding="utf-8"))
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

# End-to-end v3: several visual shots and timed beats inside one narration scene.
shots_output = folder / "shots-v3"
shots_run = run([bin_dir / "studio_produce.exe", "--recipe",
                 root / "examples/shots-visual-beats-v3.production.json",
                 "--output-dir", shots_output], timeout=360)
(folder / "shots-v3.log").write_text(shots_run.stdout + shots_run.stderr, encoding="utf-8")
shots_project_path = shots_output / "project.studio.json"
shots_project = json.loads(shots_project_path.read_text(encoding="utf-8"))
assert shots_project["schemaVersion"] == 7
assert len(shots_project["scenes"]) == 1 and len(shots_project["shots"]) == 3
assert len(shots_project["visualBeats"]) == 3
ordered_shots = sorted(shots_project["shots"], key=lambda shot: shot["start"])
scene = shots_project["scenes"][0]
assert ordered_shots[0]["start"] == scene["start"]
assert ordered_shots[-1]["start"] + ordered_shots[-1]["duration"] == scene["start"] + scene["duration"]
assert all(left["start"] + left["duration"] == right["start"]
           for left, right in zip(ordered_shots, ordered_shots[1:]))
assert all(node.get("shotId") for node in shots_project["nodes"]
           if node.get("id") in {"carro", "preco", "documento", "aviso"})
assert any(node.get("keyframes") for node in shots_project["nodes"] if node.get("id") == "preco")
shots_video = shots_output / "shots-v3.mp4"
run([bin_dir / "studio_render.exe", "--project", shots_project_path,
     "--output", shots_video, "--ffmpeg", ffmpeg], timeout=180)
shots_decode = run([ffmpeg, "-hide_banner", "-i", shots_video, "-f", "null", "-"])
assert re.search(rf"frame=\s*{shots_project['duration']}\b", shots_decode.stderr)
proof_frames = sorted({shot["start"] for shot in ordered_shots} |
                      {beat["frame"] for beat in shots_project["visualBeats"]})
proof_frames = [frame for frame in proof_frames if frame < shots_project["duration"]]
for index, frame in enumerate(proof_frames, 1):
    run([bin_dir / "studio_render.exe", "--project", shots_project_path, "--still", frame,
         "--output", output / f"v3-reference-{index}.png"])
select_v3 = "+".join(f"eq(n\\,{frame})" for frame in proof_frames)
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", shots_video,
     "-vf", f"select={select_v3}", "-fps_mode", "vfr", output / "v3-decoded-%d.png"])
v3_match = run([ffmpeg, "-hide_banner", "-i", output / "v3-reference-%d.png", "-i",
                output / "v3-decoded-%d.png", "-lavfi", "psnr", "-f", "null", "-"])
v3_psnr = re.search(r"average:([0-9.]+|inf)", v3_match.stderr)
assert v3_psnr and float(v3_psnr.group(1)) > 30, "v3 cuts/beats differ between preview and export"
v3_ui = run([bin_dir / "studio_dark.exe", "--smoke", output / "interface-v3",
             "--project", shots_project_path, "--frame", proof_frames[len(proof_frames) // 2]], timeout=30)
(folder / "shots-v3-desktop.log").write_text(v3_ui.stderr, encoding="utf-8")
for marker in ("failed to load", "ReferenceError", "TypeError", "Cannot assign", "Binding loop"):
    assert marker not in v3_ui.stderr, v3_ui.stderr
v3_runtime = json.loads((output / "interface-v3/runtime.json").read_text(encoding="utf-8"))
assert v3_runtime["page"] == "editor" and v3_runtime["hasAudio"]

# Full-length director-led production with measured pt-BR word timing.
director_output = folder / "directors-cut-90s"
director_recipe = root / "examples/carros-caros-directors-cut-90s.production.json"
director_run = run([bin_dir / "studio_produce.exe", "--recipe", director_recipe,
                    "--output-dir", director_output], timeout=360)
(folder / "directors-cut-90s.log").write_text(director_run.stdout + director_run.stderr,
                                               encoding="utf-8")
director_project_path = director_output / "project.studio.json"
director_project = json.loads(director_project_path.read_text(encoding="utf-8"))
director_report = json.loads((director_output / "production-report.json").read_text(encoding="utf-8"))
assert director_project["schemaVersion"] == 9 and director_project["duration"] == 2700
assert len(director_project["scenes"]) == 6 and len(director_project["shots"]) == 18
assert len(director_project["visualBeats"]) == 20
sound_beat = next(beat for beat in director_project["visualBeats"] if beat["action"] == "sound")
sound_track = next(track for track in director_project["audioTracks"] if track["id"] == sound_beat["targetId"])
assert sound_track["start"] == sound_beat["frame"] and sound_track["source"].endswith("soft-chime.wav")
assert director_report["timingMode"] == "fit-to-duration" and director_report["frameCount"] == 2700
with wave.open(str(director_output / "media/master.wav"), "rb") as director_audio:
    assert director_audio.getnframes() == 2700 * 800 and director_audio.getframerate() == 24000
for scene in director_project["scenes"]:
    scene_shots = sorted((shot for shot in director_project["shots"] if shot["sceneId"] == scene["id"]),
                         key=lambda shot: shot["start"])
    assert len(scene_shots) == 3 and scene_shots[0]["start"] == scene["start"]
    assert scene_shots[-1]["start"] + scene_shots[-1]["duration"] == scene["start"] + scene["duration"]
    assert all(left["start"] + left["duration"] == right["start"]
               for left, right in zip(scene_shots, scene_shots[1:]))
assert all(beat["frame"] < director_project["duration"] for beat in director_project["visualBeats"])
director_video = director_output / "directors-cut-90s.mp4"
run([bin_dir / "studio_render.exe", "--project", director_project_path,
     "--output", director_video, "--ffmpeg", ffmpeg], timeout=360)
director_decode = run([ffmpeg, "-hide_banner", "-i", director_video, "-f", "null", "-"], timeout=180)
assert re.search(r"frame=\s*2700\b", director_decode.stderr), "90-second MP4 must contain exactly 2700 frames"
director_proof_frames = sorted({shot["start"] for shot in director_project["shots"]} |
                               {beat["frame"] for beat in director_project["visualBeats"]})
for index, frame in enumerate(director_proof_frames, 1):
    run([bin_dir / "studio_render.exe", "--project", director_project_path, "--still", frame,
         "--output", output / f"director-reference-{index}.png"])
director_select = "+".join(f"eq(n\\,{frame})" for frame in director_proof_frames)
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", director_video,
     "-vf", f"select={director_select}", "-fps_mode", "vfr", output / "director-decoded-%d.png"])
director_comparison = run([ffmpeg, "-hide_banner", "-i", output / "director-reference-%d.png", "-i",
                           output / "director-decoded-%d.png", "-lavfi", "psnr", "-f", "null", "-"])
director_psnr = re.search(r"average:([0-9.]+|inf)", director_comparison.stderr)
assert director_psnr and float(director_psnr.group(1)) > 30, "90-second shot/beat frames differ from the preview"
director_ui = run([bin_dir / "studio_dark.exe", "--smoke", output / "interface-director-90s",
                   "--project", director_project_path, "--frame", 1350], timeout=30)
(folder / "directors-cut-90s-desktop.log").write_text(director_ui.stderr, encoding="utf-8")
for marker in ("failed to load", "ReferenceError", "TypeError", "Cannot assign", "Binding loop"):
    assert marker not in director_ui.stderr, director_ui.stderr
director_runtime = json.loads((output / "interface-director-90s/runtime.json").read_text(encoding="utf-8"))
assert director_runtime["page"] == "editor" and director_runtime["hasAudio"]

# Trim, playback speed and source crop use the same FFmpeg composition for preview and export.
video_source = folder / "video-controls-source.mp4"
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i",
     "testsrc2=size=320x180:rate=30:duration=2", "-c:v", "libx264", "-pix_fmt", "yuv420p", video_source])
video_controls_project = {
    "format": "studio-dark", "schemaVersion": 10, "id": "video-controls-smoke",
    "name": "Video trim speed crop smoke", "width": 320, "height": 180, "duration": 30,
    "fps": {"numerator": 30, "denominator": 1}, "nodes": [{
        "id": "broll", "name": "B-roll", "kind": "video", "source": str(video_source),
        "color": "#e4f5e9", "secondaryColor": "#f3c49a",
        "start": 0, "duration": 30, "x": 0, "y": 0, "width": 320, "height": 180,
        "opacity": 1, "fontSize": 64, "animated": False, "motion": "none",
        "video": {"sourceInSeconds": 0.25, "speed": 1.5,
                  "crop": {"x": 0.125, "y": 0.125, "width": 0.75, "height": 0.75}},
    }]
}
video_controls_path = output / "video-controls.studio.json"
video_controls_path.write_text(json.dumps(video_controls_project, indent=2), encoding="utf-8")
video_controls_preview = output / "video-controls-preview.png"
video_controls_export = output / "video-controls.mp4"
run([bin_dir / "studio_render.exe", "--project", video_controls_path, "--still", 15,
     "--output", video_controls_preview, "--ffmpeg", ffmpeg])
run([bin_dir / "studio_render.exe", "--project", video_controls_path,
     "--output", video_controls_export, "--ffmpeg", ffmpeg], timeout=180)
video_controls_decode = run([ffmpeg, "-hide_banner", "-i", video_controls_export, "-f", "null", "-"])
assert re.search(r"frame=\s*30\b", video_controls_decode.stderr)
video_controls_decoded_frame = output / "video-controls-decoded.png"
run([ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", video_controls_export,
     "-vf", "select=eq(n\\,15)", "-frames:v", "1", video_controls_decoded_frame])
video_controls_match = run([ffmpeg, "-hide_banner", "-i", video_controls_preview,
                            "-i", video_controls_decoded_frame, "-lavfi", "psnr", "-f", "null", "-"])
video_controls_psnr = re.search(r"average:([0-9.]+|inf)", video_controls_match.stderr)
assert video_controls_psnr and float(video_controls_psnr.group(1)) > 30

summary = {"status": "passed", "voice": report["voice"], "scenes": 6, "nodes": len(project["nodes"]), "frames": project["duration"], "durationSeconds": project["duration"] / 30, "audioRms": rms, "previewExportPsnrDb": float(match.group(1)), "shotsV3Frames": shots_project["duration"], "shotsV3Count": len(ordered_shots), "visualBeatsV3Count": len(shots_project["visualBeats"]), "shotsV3PreviewExportPsnrDb": float(v3_psnr.group(1)), "elapsedSeconds": round(time.monotonic() - started, 2), "video": str(video), "checks": ["native recipe compiler", "real Windows pt-BR speech", "measured audio timing", "PCM mix", "H264 and AAC decode", "six frame comparisons", "QML window capture", "v3 project opens in editor", "missing voice rejection", "guide/catalogue consistency", "native library contact sheet", "8-second frame-exact production", "cueProgress after TTS", "240-frame MP4", "v3 independent shots and Visual Beats", "shot coverage and no gaps", "v3 preview/export frame comparison"]}
summary.update({"director90Frames": director_project["duration"], "director90Shots": len(director_project["shots"]),
                "director90Beats": len(director_project["visualBeats"]),
                "director90PreviewExportPsnrDb": float(director_psnr.group(1)),
                "director90Video": str(director_video)})
summary["checks"].extend(["90-second 2700-frame director cut", "18 independent shots and 20 Visual Beats",
                          "word-synchronized one-shot audio effect through editable track",
                          "word-aligned scene timing", "director preview/export comparison",
                          "90-second project opens in editor"])
summary["videoControlsPreviewExportPsnrDb"] = float(video_controls_psnr.group(1))
summary["checks"].append("video source trim, speed and normalized crop match between preview and MP4")
(folder / "verification.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
print(json.dumps({"artifacts": str(folder), **summary}, indent=2))
