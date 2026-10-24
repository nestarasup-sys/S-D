param([switch]$Test)
. "$PSScriptRoot/environment.ps1"
foreach ($StudioFile in @($StudioCmake, $StudioNinja, "$StudioMingw/g++.exe", "$StudioQt/bin/Qt6Quick.dll")) {
    if (!(Test-Path -LiteralPath $StudioFile)) { throw "Ferramenta ausente: $StudioFile. Execute scripts/setup.ps1." }
}
$StudioRunningPath = Join-Path $StudioBin 'studio_dark.exe'
$StudioRunning = Get-Process studio_dark -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $StudioRunningPath }
if ($StudioRunning) {
    $StudioBuild = Join-Path $StudioRoot ('build/session-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
    Write-Host 'O editor esta aberto. Compilando em uma pasta nova para preservar a sessao.'
}
& $StudioCmake -S $StudioRoot -B $StudioBuild -G Ninja "-DCMAKE_MAKE_PROGRAM=$StudioNinja" "-DCMAKE_CXX_COMPILER=$StudioMingw/g++.exe" "-DCMAKE_PREFIX_PATH=$StudioQt" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'Configuração CMake falhou.' }
& $StudioCmake --build $StudioBuild --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Compilação falhou.' }
if ($Test) {
    & $StudioCtest --test-dir $StudioBuild --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Testes falharam.' }
}
[System.IO.Directory]::CreateDirectory((Join-Path $StudioRoot 'build')) | Out-Null
[System.IO.File]::WriteAllText($StudioActiveBuild,$StudioBuild,[System.Text.UTF8Encoding]::new($false))
$env:STUDIO_BIN_DIR = Join-Path $StudioBuild 'bin'
