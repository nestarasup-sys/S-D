. "$PSScriptRoot/environment.ps1"
python "$PSScriptRoot/smoke-workspace.py" "$StudioRoot"
if ($LASTEXITCODE -ne 0) { throw 'Smoke de projetos e biblioteca falhou.' }
