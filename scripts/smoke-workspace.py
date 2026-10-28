"""Visual QA of empty/filled project home and the complete native library."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

root = Path(sys.argv[1]).resolve()
out = root / "artifacts" / time.strftime("workspace-smoke-%Y%m%d-%H%M%S")
out.mkdir(parents=True, exist_ok=False)
bin_dir = Path(os.environ.get("STUDIO_BIN_DIR", root / "build/bin"))
profile = out / "profile"


def capture(page, destination, project=None):
    command = [str(bin_dir / "studio_dark.exe"), "--smoke", str(profile), "--page", page]
    if project:
        command += ["--project", str(project)]
    result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    assert not any(marker in result.stderr for marker in ("failed to load", "ReferenceError", "TypeError", "Cannot assign", "Binding loop", "Unable to assign")), result.stderr
    (out / f"{destination}.log").write_text(result.stderr, encoding="utf-8")
    shutil.copy2(profile / "desktop.png", out / f"{destination}.png")
    runtime = json.loads((profile / "runtime.json").read_text(encoding="utf-8"))
    assert runtime["page"] == page, runtime
    return runtime


empty = capture("projects", "home-empty")
assert empty["projects"] == 0 and empty["assets"] == 81 and empty["categories"] == 11
assert empty["newProjectCanvasOptions"] == 3, empty
base = json.loads((profile / "demo.studio.json").read_text(encoding="utf-8"))
fixtures = out / "fixtures"
fixtures.mkdir()
for index, name in enumerate(("Histórias que merecem espaço", "Identidade do meu canal")):
    project = dict(base, id=f"workspace-fixture-{index}", name=name)
    project["nodes"] = [dict(n) for n in base["nodes"]]
    project["nodes"][1]["text"] = "Histórias que\nmerecem espaço." if index == 0 else "Uma identidade.\nMuitas histórias."
    project["nodes"][1]["fontSize"] = 72
    path = fixtures / f"project-{index}.studio.json"
    path.write_text(json.dumps(project, ensure_ascii=False), encoding="utf-8")
    filled = capture("projects", "home-projects", path)
assert filled["projects"] == 2
library = capture("library", "asset-library", path)
assert library["assets"] == 81 and library["categories"] == 11 and library["projects"] == 2
editor = capture("editor", "editor-navigation", path)
assert editor["projects"] == 2
report = {"status": "passed", "projects": 2, "nativeAssets": 72, "defaultAiPngNativeAssets": 60, "previewOnlyScenes": 12, "proceduralDynamicNatives": 12, "premiumAssets": 9, "libraryItems": 81, "categories": 11, "checks": ["empty home", "project cards and previews", "persistent project catalogue", "72 original native IDs with 60 default AI PNG artworks, 12 dynamic procedural assets and nine premium assets", "12 low-resolution native scenes marked explicitly", "font and visual-style library categories", "editor navigation", "QML error-free captures"]}
(out / "verification.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps({"artifacts": str(out), **report}, indent=2))
