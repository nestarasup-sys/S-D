$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment.ps1"
$test = Join-Path $StudioBin 'studio_visual_beat_tests.exe'
if (!(Test-Path -LiteralPath $test)) { throw 'Execute scripts/build.ps1 antes do teste Visual Beats.' }
& $StudioCtest --test-dir $StudioBuild --output-on-failure -R '^production_visual_beat_temporal$'
if ($LASTEXITCODE -ne 0) { throw 'A regressão temporal/visual dos Visual Beats falhou.' }
$case = Get-ChildItem (Join-Path $StudioRoot 'artifacts') -Directory |
    Where-Object Name -Like 'visual-beats-regression-*' |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (!$case) { throw 'O teste não gerou sua fixture de evidências.' }
$caseReport = Get-Content (Join-Path $case.FullName 'report.json') -Raw | ConvertFrom-Json
$movie = Join-Path $case.FullName 'visual-beats-reveal.mp4'
$render = Join-Path $StudioBin 'studio_render.exe'
& $render --project $caseReport.savedProject --output $movie --ffmpeg $StudioEncoder.FullName
if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $movie)) { throw 'Exportação MP4 da fixture Visual Beats falhou.' }
$decodeDir = Join-Path $case.FullName 'mp4-frames'
[System.IO.Directory]::CreateDirectory($decodeDir) | Out-Null
& $StudioEncoder.FullName -y -hide_banner -loglevel error -i $movie -fps_mode passthrough `
    (Join-Path $decodeDir 'frame-%03d.png')
if ($LASTEXITCODE -ne 0) { throw 'Não foi possível decodificar o MP4 da fixture.' }
$videoCheck = Join-Path $PSScriptRoot 'verify-visual-beat-mp4.py'
& python $videoCheck --frames $decodeDir --report (Join-Path $case.FullName 'report.json') `
    --output (Join-Path $case.FullName 'mp4-visual-check.json')
if ($LASTEXITCODE -ne 0) { throw 'Os objetos não foram confirmados nos quadros MP4 esperados.' }
Write-Host "Visual Beats: QPainter, save/reopen e MP4 verificados em $($case.FullName)"
