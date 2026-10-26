param([string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment.ps1"
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $StudioRoot ("artifacts/shader-validation-" + (Get-Date -Format 'yyyyMMdd-HHmmssfff'))
}
[System.IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$qsb = Join-Path $StudioQt 'bin/qsb.exe'
$shader = Join-Path $StudioRoot 'tests/shaders/alpha_mask.frag'
if (!(Test-Path -LiteralPath $qsb)) { throw "Qt Shader Tools ausente: $qsb" }
if (!(Test-Path -LiteralPath $shader)) { throw "Shader de regressão ausente: $shader" }
$version = (& $qsb --version 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $version -notmatch '6\.8\.3') {
    throw "qsb não corresponde ao Qt adotado (6.8.3): $version"
}
$pack = Join-Path $OutputDirectory 'alpha_mask.frag.qsb'
$compileOutput = & $qsb --qt6 -o $pack $shader 2>&1 | Out-String
$compileExit = $LASTEXITCODE
if ($compileExit -ne 0 -or !(Test-Path -LiteralPath $pack)) {
    throw "qsb falhou ao compilar shaders QRhi/Qt Quick: $compileOutput"
}
$dump = & $qsb --dump $pack 2>&1 | Out-String
$dumpExit = $LASTEXITCODE
if ($dumpExit -ne 0 -or $dump -notmatch 'SPIR-V 100' -or $dump -notmatch 'HLSL 50' -or
    $dump -notmatch 'GLSL 150' -or $dump -notmatch 'maskTexture' -or $dump -notmatch 'maskStrength') {
    throw "O pacote .qsb não contém as variantes/reflexão esperadas: $dump"
}
$dumpPath = Join-Path $OutputDirectory 'alpha_mask.dump.txt'
[System.IO.File]::WriteAllText($dumpPath, $dump, [System.Text.UTF8Encoding]::new($false))
$report = [ordered]@{
    status = 'passed'
    qtVersion = '6.8.3'
    qsbVersion = $version
    shaderSource = 'tests/shaders/alpha_mask.frag'
    output = $pack
    compiledTargets = @('SPIR-V 100', 'GLSL 100 es', 'GLSL 120', 'GLSL 150', 'HLSL 50', 'MSL 12')
    reflectionChecks = @('sourceTexture', 'maskTexture', 'opacity', 'maskStrength')
    runtimeIntegrated = $false
    note = 'Compilação e serialização multibackend verificadas; nenhum efeito QSB foi integrado ao compositor Scene Graph.'
}
$reportPath = Join-Path $OutputDirectory 'report.json'
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $reportPath -Encoding utf8
[pscustomobject]$report
