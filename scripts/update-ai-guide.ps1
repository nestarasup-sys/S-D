. "$PSScriptRoot/environment.ps1"
& "$StudioBin/studio_produce.exe" --guide "$StudioRoot/docs/GUIA-PARA-IA.md"
if ($LASTEXITCODE -ne 0) { throw 'Falha ao gerar guia.' }
& python "$StudioRoot/scripts/embed-ai-production-example.py" --guide "$StudioRoot/docs/GUIA-PARA-IA.md" --example "$StudioRoot/examples/carros-caros-directors-cut-90s.production.json"
if ($LASTEXITCODE -ne 0) { throw 'Falha ao inserir o exemplo profissional testado no guia.' }
& "$StudioBin/studio_produce.exe" --capabilities "$StudioRoot/docs/capabilities-v1.json"
if ($LASTEXITCODE -ne 0) { throw 'Falha ao gerar catalogo.' }
