. "$PSScriptRoot/environment.ps1"
& $StudioCtest --test-dir $StudioBuild --output-on-failure -R '^composition_text_temporal_stability$'
if ($LASTEXITCODE -ne 0) { throw 'A sequência temporal de texto falhou.' }
$artifact = Get-ChildItem -LiteralPath (Join-Path $StudioRoot 'artifacts') -Directory |
    Where-Object Name -Like 'text-temporal-*' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $artifact) { throw 'O teste temporal não gerou evidências.' }
$project = Join-Path $artifact.FullName 'text-pan-zoom.studio.json'
$render = Join-Path $StudioBin 'studio_render.exe'
foreach ($frame in @(0, 15, 30, 45, 59)) {
    & $render --project $project --still $frame `
        --output (Join-Path $artifact.FullName ("windows-reference-{0:D3}.png" -f $frame))
    if ($LASTEXITCODE -ne 0) { throw "Falha ao capturar PNG de referência do quadro $frame." }
}
& $render --project $project `
    --output (Join-Path $artifact.FullName 'text-temporal-check.mp4') --ffmpeg $StudioEncoder.FullName
if ($LASTEXITCODE -ne 0) { throw 'A exportação MP4 de texto falhou.' }
python "$PSScriptRoot/verify-text-mp4.py" $artifact.FullName $StudioEncoder.FullName
if ($LASTEXITCODE -ne 0) { throw 'A comparação de texto em PNG/MP4 falhou.' }
Write-Host "Evidências temporais: $($artifact.FullName)"
