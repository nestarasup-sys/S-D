param()
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment.ps1"
$exe = Join-Path $StudioBin 'studio_large_project_tests.exe'
if (!(Test-Path -LiteralPath $exe)) { throw 'Benchmark ausente; execute scripts/build.ps1 primeiro.' }
$env:QT_QPA_PLATFORM = 'offscreen'
& $exe
if ($LASTEXITCODE -ne 0) { throw 'O teste progressivo de projetos grandes falhou.' }
$report = Join-Path $StudioRoot 'artifacts/large-project-scaling/report.json'
if (!(Test-Path -LiteralPath $report)) { throw 'O executável terminou sem gerar report.json.' }
Write-Host "Medições salvas em $report"
