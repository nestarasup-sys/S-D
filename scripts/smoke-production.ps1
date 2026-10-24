. "$PSScriptRoot/environment.ps1"
python "$PSScriptRoot/smoke-production.py" "$StudioRoot"
if ($LASTEXITCODE -ne 0) { throw 'Smoke da producao audiovisual falhou.' }
