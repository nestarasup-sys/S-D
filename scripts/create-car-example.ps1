param([switch]$NoOpen)
. "$PSScriptRoot/environment.ps1"
if (!(Test-Path "$StudioBin/studio_produce.exe")) { & "$PSScriptRoot/build.ps1" -Test; . "$PSScriptRoot/environment.ps1" }
$StudioOutput = Join-Path $StudioRoot ('artifacts/carros-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
Write-Host 'Gerando voz em portugues e montando as cenas...'
& "$StudioBin/studio_produce.exe" --recipe "$StudioRoot/examples/carros-caros.production.json" --output-dir $StudioOutput
if ($LASTEXITCODE -ne 0) { throw 'Nao foi possivel montar o video. O projeto anterior foi preservado.' }
Write-Host 'Exportando o MP4 com narracao e trilha...'
& "$StudioBin/studio_render.exe" --project "$StudioOutput/project.studio.json" --output "$StudioOutput/carros-caros.mp4" --ffmpeg $env:STUDIO_FFMPEG
if ($LASTEXITCODE -ne 0) { throw 'Falha ao exportar. O projeto editavel permanece na pasta de producao.' }
Write-Host "Video pronto em $StudioOutput/carros-caros.mp4"
if (!$NoOpen) {
    $StudioArguments = '--project "' + "$StudioOutput/project.studio.json" + '" --frame 90'
    Start-Process -FilePath "$StudioBin/studio_dark.exe" -ArgumentList $StudioArguments -WorkingDirectory $StudioRoot -WindowStyle Normal
}
