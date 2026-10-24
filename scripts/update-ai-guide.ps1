. "$PSScriptRoot/environment.ps1"
& "$StudioBin/studio_produce.exe" --guide "$StudioRoot/docs/GUIA-PARA-IA.md"
if ($LASTEXITCODE -ne 0) { throw 'Falha ao gerar guia.' }
& "$StudioBin/studio_produce.exe" --capabilities "$StudioRoot/docs/capabilities-v1.json"
if ($LASTEXITCODE -ne 0) { throw 'Falha ao gerar catalogo.' }
