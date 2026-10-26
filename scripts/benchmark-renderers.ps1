param([int]$Frames = 60, [int]$Nodes = 120, [ValidateRange(1, 5)][int]$Repeats = 3)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment.ps1"
$prototype = Join-Path $StudioBin 'studio_scenegraph_prototype.exe'
if (!(Test-Path -LiteralPath $prototype)) { throw 'Protótipo ausente; execute scripts/build.ps1 primeiro.' }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmssfff'
$outputRoot = Join-Path $StudioRoot "artifacts/renderer-benchmark-$stamp"
[System.IO.Directory]::CreateDirectory($outputRoot) | Out-Null
$profiles = @(
    @{ name = '1280x720'; width = 1280; height = 720 },
    @{ name = '1920x1080'; width = 1920; height = 1080 },
    @{ name = '1080x1920'; width = 1080; height = 1920 }
)
$results = @()
$measurements = @()
function Get-Median([double[]]$Values) {
    $sorted = @($Values | Sort-Object)
    $middle = [int][math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2) { return [double]$sorted[$middle] }
    return ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2
}
foreach ($profile in $profiles) {
    $trials = @()
    for ($trial = 1; $trial -le $Repeats; $trial++) {
        $out = Join-Path (Join-Path $outputRoot $profile.name) ("trial-{0:D2}" -f $trial)
        Write-Host "Benchmark $($profile.width)x$($profile.height), tentativa $trial/$Repeats, $Nodes formas, $Frames quadros..."
        & $prototype --width $profile.width --height $profile.height --frames $Frames --nodes $Nodes --output $out
        if ($LASTEXITCODE -ne 0) { throw "Benchmark falhou em $($profile.name), tentativa $trial, código $LASTEXITCODE." }
        $trials += Get-Content -LiteralPath (Join-Path $out 'metrics.json') -Raw | ConvertFrom-Json
    }
    $results += [ordered]@{
        canvas = @{ width = $profile.width; height = $profile.height }
        trials = $trials
        cpuPainterMsPerFrameMedian = Get-Median @($trials | ForEach-Object { [double]$_.cpuPainterMsPerFrame })
        sceneGraphMsPerFrameMedian = Get-Median @($trials | ForEach-Object { [double]$_.sceneGraphMsPerFrame })
        visualPsnrDbMedian = Get-Median @($trials | ForEach-Object { [double]$_.visualPsnrDb })
        cpuWorkingSetBytesMedian = Get-Median @($trials | ForEach-Object { [double]$_.cpuWorkingSetBytes })
        sceneGraphWorkingSetBytesMedian = Get-Median @($trials | ForEach-Object { [double]$_.sceneGraphWorkingSetBytes })
    }
    $measurements += $trials
}
$report = [ordered]@{
    status = 'passed'
    date = (Get-Date).ToString('yyyy-MM-dd HH:mm:ss zzz')
    rendererPrototype = 'Qt Quick Scene Graph / Direct3D11 when available'
    comparator = 'CompositionPainter / QPainter to QImage'
    shapeCount = $Nodes
    framesPerResolution = $Frames
    repetitionsPerResolution = $Repeats
    everyVisualComparisonPassed = -not [bool](@($measurements | Where-Object { !$_.visualRegressionPassed }).Count)
    profiles = $results
}
$reportPath = Join-Path $outputRoot 'report.json'
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $reportPath -Encoding utf8
Write-Host "Relatório: $reportPath"
$report | ConvertTo-Json -Depth 8
