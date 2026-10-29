. "$PSScriptRoot/environment.ps1"
python "$PSScriptRoot/smoke-ui-components.py" "$StudioRoot"
if ($LASTEXITCODE -ne 0) { throw 'Smoke dos componentes UI falhou.' }
