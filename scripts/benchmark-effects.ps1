param([ValidateRange(1, 5)][int]$Repeats = 3)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment.ps1"
$test = Join-Path $StudioBin 'studio_effect_pipeline_tests.exe'
if (!(Test-Path -LiteralPath $test)) { throw 'Teste de efeitos ausente; execute scripts/build.ps1 primeiro.' }
$profiles = @(
    @{ name = '1280x720'; width = 1280; height = 720 },
    @{ name = '1920x1080'; width = 1920; height = 1080 },
    @{ name = '1080x1920'; width = 1080; height = 1920 }
)
$stamp = Get-Date -Format 'yyyyMMdd-HHmmssfff'
$outputRoot = Join-Path $StudioRoot "artifacts/effect-benchmark-$stamp"
[System.IO.Directory]::CreateDirectory($outputRoot) | Out-Null
$results = @()
$gpuRuntimeTest = Join-Path $StudioBin 'studio_gpu_effect_tests.exe'
if (!(Test-Path -LiteralPath $gpuRuntimeTest)) { throw 'Teste Scene Graph de efeitos GPU ausente; execute scripts/build.ps1 primeiro.' }
& $gpuRuntimeTest
if ($LASTEXITCODE -ne 0) { throw 'Benchmark/runtime GPU falhou; consulte artifacts/gpu-effects-test/report.json.' }
$gpuRuntimeReport = Join-Path $StudioRoot 'artifacts/gpu-effects-test/report.json'
if (!(Test-Path -LiteralPath $gpuRuntimeReport)) { throw 'O teste GPU não gerou relatório de runtime.' }
$gpuRuntimeMetrics = Get-Content -LiteralPath $gpuRuntimeReport -Raw | ConvertFrom-Json
if ($gpuRuntimeMetrics.status -ne 'passed' -or $gpuRuntimeMetrics.runtimeGpuEffects -ne $true) {
    throw 'O relatório não confirma execução real dos efeitos no backend GPU.'
}
foreach ($profile in $profiles) {
    $trials = @()
    for ($trial = 1; $trial -le $Repeats; $trial++) {
        $output = & $test --benchmark $profile.width $profile.height 2>&1 | Out-String
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) { throw "Efeitos falharam em $($profile.name), repetição $trial`:`n$output" }
        $metric = $output | ConvertFrom-Json
        if ($metric.scratchBytes -gt $metric.workingSetLimitBytes -or $metric.passes.Count -ne 3) {
            throw "Orçamento ou pilha de passes inválida em $($profile.name)."
        }
        $trials += $metric
    }
    $stat = {
        param([double[]]$Values)
        $sorted = @($Values | Sort-Object)
        $middle = [int][math]::Floor($sorted.Count / 2)
        $median = if ($sorted.Count % 2) { [double]$sorted[$middle] } else {
            ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2
        }
        return [ordered]@{ median = $median; min = $sorted[0]; max = $sorted[-1] }
    }
    $results += [ordered]@{
        profile = $profile.name
        canvas = @{ width = $profile.width; height = $profile.height }
        repeats = $Repeats
        benchmarkBackend = 'QPainter CPU fallback'
        gaussianBlurMs = & $stat @($trials | ForEach-Object { [double]$_.passes[0].elapsedMilliseconds })
        outerGlowMs = & $stat @($trials | ForEach-Object { [double]$_.passes[1].elapsedMilliseconds })
        alphaMaskMs = & $stat @($trials | ForEach-Object { [double]$_.passes[2].elapsedMilliseconds })
        totalEffectsMs = & $stat @($trials | ForEach-Object { [double]$_.totalPassMilliseconds })
        roiTotalMs = & $stat @($trials | ForEach-Object { [double]$_.roiTotalMilliseconds })
        roiScratchBytes = & $stat @($trials | ForEach-Object { [double]$_.roiScratchBytes })
        roiOutputMatchesFullCanvas = @($trials | Where-Object { -not $_.roiOutputMatchesFullCanvas }).Count -eq 0
        scratchBytes = & $stat @($trials | ForEach-Object { [double]$_.scratchBytes })
        trials = $trials
    }
}
$report = [ordered]@{
    status = 'passed'
    date = (Get-Date).ToString('yyyy-MM-dd HH:mm:ss zzz')
    renderer = 'Experimental CPU effect fallback using reusable QImage scratch buffers'
    cacheBytesMaximum = 64MB
    gpuRuntimeReport = $gpuRuntimeReport
    gpuRuntimeMetrics = $gpuRuntimeMetrics
    gpuTimingsInclude = @('submission CPU time', 'readback latency', 'completed GPU frame time when QRhi timestamps are supported')
    results = $results
}
$reportPath = Join-Path $outputRoot 'report.json'
$report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reportPath -Encoding utf8
Write-Host "Relatório: $reportPath"
$results | ForEach-Object {
    [pscustomobject]@{
        Canvas = $_.profile
        GaussianBlurMs = [math]::Round($_.gaussianBlurMs.median, 2)
        GlowMs = [math]::Round($_.outerGlowMs.median, 2)
        AlphaMaskMs = [math]::Round($_.alphaMaskMs.median, 2)
        TotalMs = [math]::Round($_.totalEffectsMs.median, 2)
        RoiTotalMs = [math]::Round($_.roiTotalMs.median, 2)
        RoiScratchMiB = [math]::Round($_.roiScratchBytes.median / 1MB, 2)
        ScratchMiB = [math]::Round($_.scratchBytes.median / 1MB, 2)
    }
} | Format-Table -AutoSize
