param([switch]$Force)
$ErrorActionPreference = 'Stop'
$StudioRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $StudioRoot
$env:PYTHONPATH = Join-Path $StudioRoot '.tools/python'
if ($Force -or !(Test-Path '.tools/python/aqt')) {
    python -m pip install --target .tools/python aqtinstall==3.3.0 cmake==3.31.6 ninja==1.11.1.3 imageio-ffmpeg==0.6.0
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao preparar ferramentas Python.' }
}
if ($Force -or !(Test-Path '.tools/Qt/6.8.3/mingw_64/bin/Qt6Quick.dll')) {
    python -m aqt install-qt windows desktop 6.8.3 win64_mingw -O .tools/Qt --archives qtbase qtdeclarative qtsvg qttools
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao baixar Qt.' }
}
if ($Force -or !(Test-Path '.tools/Qt/Tools/mingw1310_64/bin/g++.exe')) {
    python -m aqt install-tool windows desktop tools_mingw1310 -O .tools/Qt
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao baixar compilador.' }
}
if ($Force -or !(Test-Path '.tools/Qt/6.8.3/mingw_64/bin/Qt6Multimedia.dll')) {
    python -m aqt install-qt windows desktop 6.8.3 win64_mingw -O .tools/Qt --modules qtmultimedia --archives qtmultimedia
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao baixar Qt Multimedia.' }
}
if ($Force -or !(Test-Path '.tools/formatter/clang_format/data/bin/clang-format.exe')) {
    python -m pip install --target .tools/formatter clang-format==19.1.7
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao preparar formatador C++.' }
}
Write-Host 'Ferramentas prontas dentro de .tools.'
