param([string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment.ps1"
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $StudioRoot ("artifacts/dependency-check-" + (Get-Date -Format 'yyyyMMdd-HHmmssfff'))
}
[System.IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$qtRoot = Join-Path $StudioRoot '.tools/Qt/6.8.3/mingw_64'
$pluginRoot = Join-Path $qtRoot 'plugins'
$prototype = Join-Path $StudioBin 'studio_scenegraph_prototype.exe'
$requiredPaths = [ordered]@{
    QtCore = Join-Path $qtRoot 'bin/Qt6Core.dll'
    QtQuick = Join-Path $qtRoot 'bin/Qt6Quick.dll'
    QtSvg = Join-Path $qtRoot 'bin/Qt6Svg.dll'
    QtMultimedia = Join-Path $qtRoot 'bin/Qt6Multimedia.dll'
    QtShaderTools = Join-Path $qtRoot 'bin/Qt6ShaderTools.dll'
    WebPPlugin = Join-Path $pluginRoot 'imageformats/qwebp.dll'
    QtFfmpegMediaPlugin = Join-Path $pluginRoot 'multimedia/ffmpegmediaplugin.dll'
    QtWindowsMediaPlugin = Join-Path $pluginRoot 'multimedia/windowsmediaplugin.dll'
    qsb = Join-Path $qtRoot 'bin/qsb.exe'
    MinGW = Join-Path $StudioMingw 'g++.exe'
    CMake = $StudioCmake
    Ninja = $StudioNinja
    Prototype = $prototype
}
$missing = @($requiredPaths.GetEnumerator() | Where-Object { !(Test-Path -LiteralPath $_.Value) } |
    ForEach-Object { "$($_.Key): $($_.Value)" })
if ($missing.Count -gt 0) { throw "Dependências obrigatórias ausentes:`n$($missing -join "`n")`nExecute scripts/setup.ps1." }

$shaderOutput = Join-Path $OutputDirectory 'shader-tools'
$shaderReport = & "$PSScriptRoot/test-shaders.ps1" -OutputDirectory $shaderOutput
if (!$shaderReport -or $shaderReport.status -ne 'passed') { throw 'Teste de qsb não passou.' }

$hybridOutput = Join-Path $OutputDirectory 'hybrid-capability-probe'
$debugPluginsBefore = $env:QT_DEBUG_PLUGINS
$quickBackendBefore = $env:QT_QUICK_BACKEND
try {
    $env:QT_DEBUG_PLUGINS = '1'
    Remove-Item Env:QT_QUICK_BACKEND -ErrorAction SilentlyContinue
    $probeLog = & $prototype --width 1280 --height 720 --frames 60 --nodes 72 --hybrid-suite --output $hybridOutput 2>&1 |
        Out-String
    $probeExit = $LASTEXITCODE
} finally {
    if ($null -eq $debugPluginsBefore) { Remove-Item Env:QT_DEBUG_PLUGINS -ErrorAction SilentlyContinue }
    else { $env:QT_DEBUG_PLUGINS = $debugPluginsBefore }
    if ($null -eq $quickBackendBefore) { Remove-Item Env:QT_QUICK_BACKEND -ErrorAction SilentlyContinue }
    else { $env:QT_QUICK_BACKEND = $quickBackendBefore }
}
[System.IO.File]::WriteAllText((Join-Path $OutputDirectory 'qt-plugin-load.log'), $probeLog,
    [System.Text.UTF8Encoding]::new($false))
$metricsPath = Join-Path $hybridOutput 'metrics.json'
if ($probeExit -ne 0 -or !(Test-Path -LiteralPath $metricsPath)) {
    throw "O protótipo falhou ao carregar plugins/compor a cena híbrida. Log: $(Join-Path $OutputDirectory 'qt-plugin-load.log')"
}
$probe = Get-Content -LiteralPath $metricsPath -Raw | ConvertFrom-Json
$requiredImageFormats = @('png', 'jpeg', 'svg', 'webp')
$missingImageFormats = @($requiredImageFormats | Where-Object { !$probe.imageFormats.$_ })
if ($missingImageFormats.Count -gt 0 -or !$probe.webpAlphaTested -or !$probe.webpAlphaPreserved -or
    !$probe.mediaFormatTestsPassed -or !$probe.textureCacheTestsPassed -or !$probe.visualRegressionPassed) {
    throw "Plugins de imagem ou a composição WebP/alpha falharam. Métricas: $metricsPath"
}

$ffmpegVersion = (& $env:STUDIO_FFMPEG -version 2>&1 | Select-Object -First 1)
$ffmpegEncoders = (& $env:STUDIO_FFMPEG -hide_banner -encoders 2>&1 | Out-String)
$webpEncoderAvailable = $ffmpegEncoders -match '\blibwebp\b'
$fontFile = Join-Path $env:WINDIR 'Fonts/arial.ttf'
$fontStatus = if (Test-Path -LiteralPath $fontFile) { 'system font file available; external font is also verified by CTest' } else { 'Segoe UI/font database is tested; Arial file absent' }
$backendStatus = if ($probe.sceneGraphApi -eq 'Direct3D11') { 'Direct3D11 verified' } else {
    "backend reported as $($probe.sceneGraphApi); compare with the required Direct3D11 device"
}
$qmake = Join-Path $qtRoot 'bin/qmake.exe'
$qtVersion = if (Test-Path -LiteralPath $qmake) { (& $qmake -query QT_VERSION 2>&1 | Out-String).Trim() } else { '6.8.3 (pinned install path)' }
$report = [ordered]@{
    status = 'passed'
    qtVersion = $qtVersion
    qtRoot = $qtRoot
    qmlImportPath = $env:QML_IMPORT_PATH
    pluginPath = $env:QT_PLUGIN_PATH
    qpaPlatformPlugin = Join-Path $pluginRoot 'platforms/qwindows.dll'
    moduleFiles = $requiredPaths
    imageFormats = $probe.imageFormats
    webp = [ordered]@{
        plugin = 'plugins/imageformats/qwebp.dll'
        loadedAndUsed = [bool]$probe.imageFormats.webpAvailable
        realFileDecode = [bool]$probe.imageFormats.webp
        alphaPreserved = [bool]$probe.webpAlphaPreserved
        compositorTexture = [bool]$probe.visualRegressionPassed
        qtPluginDebugLog = 'qt-plugin-load.log'
    }
    shaders = [ordered]@{
        qsb = $shaderReport.qsbVersion
        compiledTargets = $shaderReport.compiledTargets
        runtimeIntegrated = $false
    }
    graphicsBackend = $probe.sceneGraphApi
    graphicsBackendStatus = $backendStatus
    softwareBackend = 'unsupported for current QSGVertexColorMaterial path'
    fonts = $fontStatus
    compiler = (& (Join-Path $StudioMingw 'g++.exe') --version 2>&1 | Select-Object -First 1)
    cmake = (& $StudioCmake --version 2>&1 | Select-Object -First 1)
    ninja = (& $StudioNinja --version 2>&1 | Select-Object -First 1)
    ffmpeg = $ffmpegVersion
    ffmpegWebPEncoder = [bool]$webpEncoderAvailable
    videoPlugins = @('ffmpegmediaplugin.dll', 'windowsmediaplugin.dll')
    nonBlockingNotes = @(
        'Vulkan headers/SDK are not installed; D3D11 is the selected experiment backend.',
        'ShaderTools compiles multibackend .qsb; no shader is wired into the production editor or hybrid prototype yet.',
        'Glow, blur and alpha masks are implemented as experimental CPU effect passes; JSON/AI recipe contract is unchanged.'
    )
    hybridMetrics = $metricsPath
    shaderReport = Join-Path $shaderOutput 'report.json'
}
$reportPath = Join-Path $OutputDirectory 'environment-report.json'
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $reportPath -Encoding utf8
Write-Host "Diagnóstico: $reportPath"
$report | ConvertTo-Json -Depth 6
