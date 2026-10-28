param([string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment.ps1"
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $StudioRoot ("artifacts/shader-validation-" + (Get-Date -Format 'yyyyMMdd-HHmmssfff'))
}
[System.IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$qsb = Join-Path $StudioQt 'bin/qsb.exe'
if (!(Test-Path -LiteralPath $qsb)) { throw "Qt Shader Tools ausente: $qsb" }
$shaders = @(
    @{ name = 'fullscreen.vert'; checks = @('position', 'texCoordIn') },
    @{ name = 'alpha_mask.frag'; checks = @('sourceTexture', 'maskTexture', 'params') },
    @{ name = 'gaussian_blur_horizontal.frag'; checks = @('sourceTexture', 'params') },
    @{ name = 'gaussian_blur_vertical.frag'; checks = @('sourceTexture', 'params') },
    @{ name = 'glow_composite.frag'; checks = @('sourceTexture', 'blurredTexture', 'params') }
)
$version = (& $qsb --version 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $version -notmatch '6\.8\.3') {
    throw "qsb não corresponde ao Qt adotado (6.8.3): $version"
}
$shaderResults = @()
foreach ($entry in $shaders) {
    $source = Join-Path $StudioRoot "tests/shaders/$($entry.name)"
    if (!(Test-Path -LiteralPath $source)) { throw "Shader ausente: $source" }
    $pack = Join-Path $OutputDirectory "$($entry.name).qsb"
    $compileOutput = & $qsb --qt6 -o $pack $source 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $pack)) {
        throw "qsb falhou ao compilar $($entry.name): $compileOutput"
    }
    $dump = & $qsb --dump $pack 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0 -or $dump -notmatch 'SPIR-V 100' -or $dump -notmatch 'HLSL 50' -or
        $dump -notmatch 'GLSL 150') { throw "Variantes QRhi ausentes em $($entry.name): $dump" }
    foreach ($check in $entry.checks) {
        if ($dump -notmatch [regex]::Escape($check)) { throw "Recurso '$check' ausente na reflexão de $($entry.name): $dump" }
    }
    [System.IO.File]::WriteAllText((Join-Path $OutputDirectory "$($entry.name).dump.txt"), $dump,
        [System.Text.UTF8Encoding]::new($false))
    $shaderResults += [ordered]@{ shader = $entry.name; qsb = $pack; variants = 'SPIR-V/GLSL/HLSL'; checks = $entry.checks }
}
$gpuRuntimeTest = Join-Path $StudioBin 'studio_gpu_effect_tests.exe'
if (!(Test-Path -LiteralPath $gpuRuntimeTest)) { throw 'Teste de shaders GPU ausente; compile o projeto primeiro.' }
& $gpuRuntimeTest
if ($LASTEXITCODE -ne 0) { throw 'Execução runtime dos shaders GPU falhou; consulte artifacts/gpu-effects-test/report.json.' }
$gpuReportPath = Join-Path $StudioRoot 'artifacts/gpu-effects-test/report.json'
$gpuReport = Get-Content -LiteralPath $gpuReportPath -Raw | ConvertFrom-Json
if ($gpuReport.runtimeGpuEffects -ne $true -or $gpuReport.status -ne 'passed') {
    throw 'O runtime não confirmou efeitos executados por GPU.'
}
$report = [ordered]@{
    status = 'passed'
    qtVersion = '6.8.3'
    qsbVersion = $version
    shaders = $shaderResults
    compiledTargets = @('SPIR-V 100', 'GLSL 100 es', 'GLSL 120', 'GLSL 150', 'HLSL 50', 'MSL 12')
    reflectionChecks = @('sourceTexture', 'maskTexture', 'params')
    runtimeIntegrated = $true
    runtime = $gpuReportPath
    note = 'A verificação também executa máscara alfa, blur separável e glow no QQuickRhiItem isolado em Direct3D11 e compara a saída lida de volta com goldens CPU.'
}
$reportPath = Join-Path $OutputDirectory 'report.json'
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $reportPath -Encoding utf8
[pscustomobject]$report
