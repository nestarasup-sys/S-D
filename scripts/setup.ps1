param([switch]$Force)
$ErrorActionPreference = 'Stop'
$StudioRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $StudioRoot
$env:PYTHONPATH = Join-Path $StudioRoot '.tools/python'
$StudioQtRoot = Join-Path $StudioRoot '.tools/Qt'
$StudioQt = Join-Path $StudioQtRoot '6.8.3/mingw_64'
if ($Force -or !(Test-Path '.tools/python/aqt')) {
    python -m pip install --target .tools/python aqtinstall==3.3.0 cmake==3.31.6 ninja==1.11.1.3 imageio-ffmpeg==0.6.0
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao preparar ferramentas Python.' }
}
if ($Force -or !(Test-Path (Join-Path $StudioQt 'bin/Qt6Quick.dll'))) {
    python -m aqt install-qt windows desktop 6.8.3 win64_mingw -O $StudioQtRoot --archives qtbase qtdeclarative qtsvg qttools
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao baixar Qt.' }
}
if ($Force -or !(Test-Path '.tools/Qt/Tools/mingw1310_64/bin/g++.exe')) {
    python -m aqt install-tool windows desktop tools_mingw1310 -O .tools/Qt
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao baixar compilador.' }
}
if ($Force -or !(Test-Path (Join-Path $StudioQt 'bin/Qt6Multimedia.dll'))) {
    python -m aqt install-qt windows desktop 6.8.3 win64_mingw -O $StudioQtRoot --modules qtmultimedia --archives qtmultimedia
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao baixar Qt Multimedia.' }
}
$StudioMissingQtModules = @()
if (!(Test-Path (Join-Path $StudioQt 'plugins/imageformats/qwebp.dll'))) {
    $StudioMissingQtModules += 'qtimageformats'
}
if (!(Test-Path (Join-Path $StudioQt 'bin/Qt6ShaderTools.dll')) -or
    !(Test-Path (Join-Path $StudioQt 'bin/qsb.exe'))) {
    $StudioMissingQtModules += 'qtshadertools'
}
if ($Force) { $StudioMissingQtModules = @('qtimageformats', 'qtshadertools') }
if ($StudioMissingQtModules.Count -gt 0) {
    Write-Host "Instalando módulos Qt 6.8.3 ausentes: $($StudioMissingQtModules -join ', ')"
    $StudioAqtModuleArgs = @('-m', 'aqt', 'install-qt', 'windows', 'desktop', '6.8.3', 'win64_mingw',
        '--noarchives', '--modules') + $StudioMissingQtModules + @('-O', $StudioQtRoot)
    & python @StudioAqtModuleArgs
    if ($LASTEXITCODE -ne 0) { throw "Falha ao instalar módulos Qt: $($StudioMissingQtModules -join ', ')." }
}
if ($Force -or !(Test-Path '.tools/formatter/clang_format/data/bin/clang-format.exe')) {
    python -m pip install --target .tools/formatter clang-format==19.1.7
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao preparar formatador C++.' }
}
Write-Host 'Ferramentas Qt 6.8.3, WebP e ShaderTools preparadas dentro de .tools.'
