param([string]$Project)
. "$PSScriptRoot/environment.ps1"
$StudioExe = Join-Path $StudioBin 'studio_dark.exe'
if (!(Test-Path -LiteralPath $StudioExe)) { & "$PSScriptRoot/build.ps1"; . "$PSScriptRoot/environment.ps1"; $StudioExe = Join-Path $StudioBin 'studio_dark.exe' }
if ($Project) {
    $StudioArguments = '--project "' + $Project + '" --frame 90'
    Start-Process -FilePath $StudioExe -ArgumentList $StudioArguments -WorkingDirectory $StudioRoot -WindowStyle Normal
} else {
    Start-Process -FilePath $StudioExe -WorkingDirectory $StudioRoot -WindowStyle Normal
}
