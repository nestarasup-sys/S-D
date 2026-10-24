. "$PSScriptRoot/environment.ps1"
$StudioExe = Join-Path $StudioRoot 'build/bin/studio_dark.exe'
if (!(Test-Path -LiteralPath $StudioExe)) { & "$PSScriptRoot/build.ps1" }
Start-Process -FilePath $StudioExe -WorkingDirectory $StudioRoot -WindowStyle Normal
