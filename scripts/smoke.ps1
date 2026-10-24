. "$PSScriptRoot/environment.ps1"
python "$PSScriptRoot/smoke.py" "$StudioRoot"
if ($LASTEXITCODE -ne 0) { throw 'Smoke audiovisual falhou.' }
