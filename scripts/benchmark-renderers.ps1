param(
    [int]$Frames = 60,
    [int[]]$NodeCounts = @(20, 120, 200),
    [int]$Nodes = 0,
    [ValidateRange(1, 5)][int]$Repeats = 3
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment.ps1"
$prototype = Join-Path $StudioBin 'studio_scenegraph_prototype.exe'
if (!(Test-Path -LiteralPath $prototype)) { throw 'Protótipo ausente; execute scripts/build.ps1 primeiro.' }
if ($Frames -lt 2) { throw 'Frames deve ser pelo menos 2.' }
if ($Nodes -gt 0) { $NodeCounts = @($Nodes) }
if (!$NodeCounts.Count -or @($NodeCounts | Where-Object { $_ -lt 1 }).Count) {
    throw 'NodeCounts deve conter valores positivos.'
}
$stamp = Get-Date -Format 'yyyyMMdd-HHmmssfff'
$outputRoot = Join-Path $StudioRoot "artifacts/renderer-benchmark-$stamp"
[System.IO.Directory]::CreateDirectory($outputRoot) | Out-Null
$profiles = @(
    @{ name = '1280x720'; width = 1280; height = 720 },
    @{ name = '1920x1080'; width = 1920; height = 1080 },
    @{ name = '1080x1920'; width = 1080; height = 1920 }
)
$profileReports = @()
$allMeasurements = @()
$primitiveReports = @()

function Get-Stats([double[]]$Values) {
    $sorted = @($Values | Sort-Object)
    $middle = [int][math]::Floor($sorted.Count / 2)
    $median = if ($sorted.Count % 2) { [double]$sorted[$middle] } else {
        ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2
    }
    $mean = ($Values | Measure-Object -Average).Average
    $sumSquares = 0.0
    foreach ($value in $Values) { $sumSquares += [math]::Pow($value - $mean, 2) }
    $deviation = if ($Values.Count -gt 1) { [math]::Sqrt($sumSquares / ($Values.Count - 1)) } else { 0 }
    return [ordered]@{ median = $median; min = $sorted[0]; max = $sorted[-1]; standardDeviation = $deviation }
}

function Invoke-Profile([hashtable]$Profile, [int]$Count, [string]$Name, [bool]$PrimitiveSuite,
                       [string]$PrimitiveKind = '') {
    $trials = @()
    for ($trial = 1; $trial -le $Repeats; $trial++) {
        $frameCountForRun = if ($PrimitiveKind) { 60 } else { $Frames }
        $scenario = if ($PrimitiveKind) { "reference-$PrimitiveKind" } elseif ($PrimitiveSuite) {
            'primitive-suite'
        } else { "nodes-$Count" }
        $out = Join-Path (Join-Path (Join-Path $outputRoot $Profile.name) $scenario) ("trial-{0:D2}" -f $trial)
        Write-Host "Benchmark $($Profile.width)x$($Profile.height), $scenario, tentativa $trial/$Repeats..."
        $arguments = @('--width', $Profile.width, '--height', $Profile.height, '--frames', $frameCountForRun,
            '--nodes', $Count, '--output', $out)
        if ($PrimitiveSuite) { $arguments += '--primitive-suite' }
        if ($PrimitiveKind) { $arguments += @('--primitive-kind', $PrimitiveKind) }
        & $prototype @arguments | Out-Null
        $exitCode = $LASTEXITCODE
        $metricsPath = Join-Path $out 'metrics.json'
        if (!(Test-Path -LiteralPath $metricsPath)) {
            throw "Benchmark sem métricas em $($Profile.name), $scenario, tentativa $trial, código $exitCode."
        }
        $metrics = Get-Content -LiteralPath $metricsPath -Raw | ConvertFrom-Json
        if ($exitCode -ne 0 -or !$metrics.visualRegressionPassed) {
            throw "Regressão visual falhou em $($Profile.name), $scenario, tentativa $trial (IoU=$($metrics.visualMetrics.foregroundIntersectionOverUnion), edgeF1=$($metrics.visualMetrics.edgeF1Within1Px), MAE=$($metrics.visualMetrics.meanAbsoluteColorError))."
        }
        $trials += $metrics
        $script:allMeasurements += $metrics
    }
    return [ordered]@{
        elements = $Count
        primitiveSuite = $PrimitiveSuite
        primitiveKind = $PrimitiveKind
        trials = $trials
        cpuMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.cpuPainterMsPerFrame })
        snapshotBuildMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.snapshotBuildMsPerFrame })
        backendPreparationMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphPreparationMsPerFrame })
        frameSubmitAndWaitMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphFrameAfterSnapshotAndPreparationMsPerFrame })
        captureMs = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphCaptureMs })
        cpuWorkingSetBytes = Get-Stats @($trials | ForEach-Object { [double]$_.cpuWorkingSetBytes })
        sceneGraphWorkingSetBytes = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphWorkingSetBytes })
        geometryBytes = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphGeometryBytesEstimate })
        psnrDb = Get-Stats @($trials | ForEach-Object { [double]$_.visualPsnrDb })
        colorMae = Get-Stats @($trials | ForEach-Object { [double]$_.visualMetrics.meanAbsoluteColorError })
        foregroundIou = Get-Stats @($trials | ForEach-Object { [double]$_.visualMetrics.foregroundIntersectionOverUnion })
        edgeF1Within1Px = Get-Stats @($trials | ForEach-Object { [double]$_.visualMetrics.edgeF1Within1Px })
    }
}

function Invoke-HybridProfile([hashtable]$Profile, [string]$Scenario, [string[]]$ModeArguments,
                              [int]$ModeRepeats = $Repeats, [int]$ScenarioFrames = $Frames) {
    $trials = @()
    for ($trial = 1; $trial -le $ModeRepeats; $trial++) {
        $out = Join-Path (Join-Path (Join-Path $outputRoot $Profile.name) $Scenario) ("trial-{0:D2}" -f $trial)
        Write-Host "Benchmark híbrido $($Profile.width)x$($Profile.height), $Scenario, tentativa $trial/$ModeRepeats..."
        $arguments = @('--width', $Profile.width, '--height', $Profile.height, '--frames', $ScenarioFrames,
            '--nodes', 72, '--output', $out) + $ModeArguments
        & $prototype @arguments | Out-Null
        $exitCode = $LASTEXITCODE
        $metricsPath = Join-Path $out 'metrics.json'
        if (!(Test-Path -LiteralPath $metricsPath)) {
            throw "Benchmark híbrido sem métricas em $($Profile.name), $Scenario, tentativa $trial, código $exitCode."
        }
        $metrics = Get-Content -LiteralPath $metricsPath -Raw | ConvertFrom-Json
        if ($exitCode -ne 0 -or !$metrics.visualRegressionPassed) {
            throw "Regressão híbrida em $($Profile.name), $Scenario, tentativa $trial (IoU=$($metrics.visualMetrics.foregroundIntersectionOverUnion), edgeF1=$($metrics.visualMetrics.edgeF1Within1Px), MAE=$($metrics.visualMetrics.meanAbsoluteColorError), cache=$($metrics.textureCacheWithinBudget))."
        }
        $trials += $metrics
        $script:allMeasurements += $metrics
    }
    return [ordered]@{
        scenario = $Scenario
        canvas = @{ width = $Profile.width; height = $Profile.height }
        repetitions = $ModeRepeats
        frames = $ScenarioFrames
        trials = $trials
        cpuPainterMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.cpuPainterMsPerFrame })
        snapshotBuildMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.snapshotBuildMsPerFrame })
        backendPreparationMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphPreparationMsPerFrame })
        frameSubmitAndWaitMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphFrameAfterSnapshotAndPreparationMsPerFrame })
        cpuRasterizationMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.cpuRasterizationMsPerMeasuredFrame })
        textureUploadSubmissionMsPerFrame = Get-Stats @($trials | ForEach-Object { [double]$_.textureUploadSubmissionMsPerMeasuredFrame })
        captureMs = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphCaptureMs })
        cpuWorkingSetBytes = Get-Stats @($trials | ForEach-Object { [double]$_.cpuWorkingSetBytes })
        sceneGraphWorkingSetBytes = Get-Stats @($trials | ForEach-Object { [double]$_.sceneGraphWorkingSetBytes })
        peakTextureCacheBytes = Get-Stats @($trials | ForEach-Object { [double]$_.textureCachePeakBytes })
        textureCacheEvictions = Get-Stats @($trials | ForEach-Object { [double]$_.textureCacheEvictions })
        psnrDb = Get-Stats @($trials | ForEach-Object { [double]$_.visualPsnrDb })
        colorMae = Get-Stats @($trials | ForEach-Object { [double]$_.visualMetrics.meanAbsoluteColorError })
        alphaMae = Get-Stats @($trials | ForEach-Object { [double]$_.visualMetrics.meanAbsoluteAlphaError })
        foregroundIou = Get-Stats @($trials | ForEach-Object { [double]$_.visualMetrics.foregroundIntersectionOverUnion })
        edgeF1Within1Px = Get-Stats @($trials | ForEach-Object { [double]$_.visualMetrics.edgeF1Within1Px })
    }
}

foreach ($profile in $profiles) {
    $scenarioReports = @()
    foreach ($count in $NodeCounts) {
        $scenarioReports += Invoke-Profile $profile $count 'project-shape-snapshot' $false
    }
    $primitiveReports += [ordered]@{
        canvas = @{ width = $profile.width; height = $profile.height }
        results = @(Invoke-Profile $profile 120 'primitive-suite' $true)
    }
    $profileReports += [ordered]@{
        canvas = @{ width = $profile.width; height = $profile.height }
        scenarios = $scenarioReports
    }
}

$primitiveReferenceTests = @()
$primitiveKinds = @('roundedRectangle', 'rectangle', 'ellipse', 'rectangleOutline', 'line', 'convexPath')
foreach ($kind in $primitiveKinds) {
    $primitiveReferenceTests += Invoke-Profile $profiles[0] 20 "reference-$kind" $true $kind
}

$hybridScenarios = @()
foreach ($profile in $profiles) {
    $hybridScenarios += Invoke-HybridProfile $profile 'hybrid-animated' @('--hybrid-suite')
    $hybridScenarios += Invoke-HybridProfile $profile 'hybrid-static' @('--hybrid-suite', '--static-scene')
}
$nativeAtlasScenario = Invoke-HybridProfile $profiles[0] 'native-atlas-72' @('--native-atlas')
$cachePressureScenario = Invoke-HybridProfile $profiles[0] 'cache-pressure-4mib' @('--hybrid-suite', '--cache-pressure-test') 1 60

# Qt 6.8's QSGVertexColorMaterial is only supported by the default scene graph
# backend. Probe the software backend and preserve the failure as an explicit
# compatibility result instead of counting it as a GPU benchmark pass.
$softwareOut = Join-Path $outputRoot 'software-backend-probe'
$previousQuickBackend = $env:QT_QUICK_BACKEND
try {
    $env:QT_QUICK_BACKEND = 'software'
    & $prototype --width 1280 --height 720 --frames $Frames --nodes 120 --primitive-suite --output $softwareOut | Out-Null
    $softwareExit = $LASTEXITCODE
} finally {
    $env:QT_QUICK_BACKEND = $previousQuickBackend
}
$softwareMetricsPath = Join-Path $softwareOut 'metrics.json'
$softwareStatus = 'unavailable'
if (Test-Path -LiteralPath $softwareMetricsPath) {
    $softwareMetrics = Get-Content -LiteralPath $softwareMetricsPath -Raw | ConvertFrom-Json
    if ($softwareMetrics.sceneGraphApi -eq 'Software' -and !$softwareMetrics.visualRegressionPassed -and $softwareExit -eq 6) {
        $softwareStatus = 'unsupported: QSGVertexColorMaterial produced no usable geometry on Qt Quick Software backend'
    } elseif ($softwareMetrics.sceneGraphApi -eq 'Software' -and $softwareMetrics.visualRegressionPassed -and $softwareExit -eq 0) {
        $softwareStatus = 'passed'
    } else {
        throw "Resultado inesperado no backend software: api=$($softwareMetrics.sceneGraphApi), exit=$softwareExit."
    }
}

$report = [ordered]@{
    status = 'passed'
    date = (Get-Date).ToString('yyyy-MM-dd HH:mm:ss zzz')
    rendererPrototype = 'Qt Quick Scene Graph / Direct3D11 when available'
    cpuComparator = 'CompositionPainter / QPainter to QImage; primitive-suite uses a QPainter oracle from the same resolved snapshot'
    shapeCounts = $NodeCounts
    framesPerScenario = $Frames
    repetitionsPerScenario = $Repeats
    apiCoverage = @{ direct3D11 = 'measured'; software = $softwareStatus; gpuMemory = 'DXGI QueryVideoMemoryInfo sampled from the adapter returned by the active Qt QRhi D3D11 device; unavailable is non-fatal' }
    gpuMemoryDefinitions = @{
        physicalDedicatedBytes = 'DXGI adapter descriptor capacity; not the Windows process budget'
        localAndNonLocal = 'IDXGIAdapter3 QueryVideoMemoryInfo process usage and Windows-assigned budget/reservation'
        textureEstimates = 'Application-side estimates remain separate and are not equated with DXGI CurrentUsage'
        processRam = 'Working set is reported independently from adapter usage'
    }
    gpuMemoryDiagnosticsByTrial = @($allMeasurements | ForEach-Object {
        [ordered]@{ scenario = $_.scenario; canvas = $_.canvas; diagnostics = $_.gpuMemoryDiagnostics }
    })
    profiles = $profileReports
    primitiveSuites = $primitiveReports
    primitiveReferenceTests = $primitiveReferenceTests
    hybridScenarios = $hybridScenarios
    nativeAtlas72 = $nativeAtlasScenario
    cachePressureTest = $cachePressureScenario
    everyVisualComparisonPassed = -not [bool](@($allMeasurements | Where-Object { !$_.visualRegressionPassed }).Count)
    softwareBackendProbe = if (Test-Path -LiteralPath $softwareMetricsPath) { $softwareMetrics } else { $null }
}
$reportPath = Join-Path $outputRoot 'report.json'
$report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding utf8
Write-Host "Relatório: $reportPath"
$summaryRows = @()
foreach ($profileReport in $profileReports) {
    $canvasName = "$($profileReport.canvas.width)x$($profileReport.canvas.height)"
    foreach ($scenario in $profileReport.scenarios) {
        $summaryRows += [pscustomobject]@{
            Canvas = $canvasName; Case = "$($scenario.elements) elements"; CPUms = [math]::Round($scenario.cpuMsPerFrame.median, 2)
            CPUsd = [math]::Round($scenario.cpuMsPerFrame.standardDeviation, 2); Snapshot = [math]::Round($scenario.snapshotBuildMsPerFrame.median, 2)
            GPUprepare = [math]::Round($scenario.backendPreparationMsPerFrame.median, 2); FrameWait = [math]::Round($scenario.frameSubmitAndWaitMsPerFrame.median, 2)
            Capture = [math]::Round($scenario.captureMs.median, 2); CPUramMiB = [math]::Round($scenario.cpuWorkingSetBytes.median / 1MB, 1)
            GpuProcessRAMMiB = [math]::Round($scenario.sceneGraphWorkingSetBytes.median / 1MB, 1)
            PSNR = [math]::Round($scenario.psnrDb.median, 2); IoU = [math]::Round($scenario.foregroundIou.median, 3)
            EdgeF1 = [math]::Round($scenario.edgeF1Within1Px.median, 3); ColorMAE = [math]::Round($scenario.colorMae.median, 2)
        }
    }
}
$summaryRows | Format-Table -AutoSize
@($hybridScenarios + @($nativeAtlasScenario) + @($cachePressureScenario)) | ForEach-Object {
    [pscustomobject]@{
        Canvas = "$($_.canvas.width)x$($_.canvas.height)"; Case = $_.scenario
        CPUms = [math]::Round($_.cpuPainterMsPerFrame.median, 2)
        Rasterms = [math]::Round($_.cpuRasterizationMsPerFrame.median, 2)
        Uploadms = [math]::Round($_.textureUploadSubmissionMsPerFrame.median, 4)
        Prepms = [math]::Round($_.backendPreparationMsPerFrame.median, 2)
        Waitms = [math]::Round($_.frameSubmitAndWaitMsPerFrame.median, 2)
        CacheMiB = [math]::Round($_.peakTextureCacheBytes.median / 1MB, 1)
        Evictions = [math]::Round($_.textureCacheEvictions.median)
        PSNR = [math]::Round($_.psnrDb.median, 2); IoU = [math]::Round($_.foregroundIou.median, 3)
        EdgeF1 = [math]::Round($_.edgeF1Within1Px.median, 3); ColorMAE = [math]::Round($_.colorMae.median, 2)
    }
} | Format-Table -AutoSize
Write-Host "Primitivas GPU: $($primitiveKinds -join ', ') | backend software: $softwareStatus"
