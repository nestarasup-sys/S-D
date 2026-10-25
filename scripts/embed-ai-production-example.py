"""Embed the exact, smoke-tested production recipe in the generated AI guide."""
import argparse
import json
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--guide", required=True, type=Path)
    parser.add_argument("--example", required=True, type=Path)
    args = parser.parse_args()

    guide = args.guide.read_text(encoding="utf-8")
    start = "<!-- STUDIO-DARK-VALIDATED-EXAMPLE-BEGIN -->"
    end = "<!-- STUDIO-DARK-VALIDATED-EXAMPLE-END -->"
    if guide.count(start) != 1 or guide.count(end) != 1:
        raise SystemExit("O guia gerado precisa ter exatamente um bloco para o exemplo validado.")

    recipe = json.loads(args.example.read_text(encoding="utf-8"))
    if recipe.get("format") != "studio-dark-production" or recipe.get("version") != 3:
        raise SystemExit("O exemplo incorporado não é uma receita de produção v3.")
    if recipe.get("targetDurationSeconds") != 90 or len(recipe.get("scenes", [])) != 6:
        raise SystemExit("O exemplo profissional deve manter 90 segundos e seis cenas.")

    shots = [shot for scene in recipe["scenes"] for shot in scene.get("shots", [])]
    beats = [beat for shot in shots for beat in shot.get("beats", [])]
    if len(shots) != 18 or len(beats) != 19:
        raise SystemExit("O exemplo profissional deve conter 18 shots e 19 Visual Beats.")

    block = f"{start}\n```json\n{json.dumps(recipe, ensure_ascii=False, indent=2)}\n```\n{end}"
    before, remainder = guide.split(start, 1)
    _, after = remainder.split(end, 1)
    args.guide.write_text(before + block + after, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
