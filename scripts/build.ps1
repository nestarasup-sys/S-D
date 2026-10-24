param([switch]$Test)
. "$PSScriptRoot/environment.ps1"
foreach ($StudioFile in @($StudioCmake, $StudioNinja, "$StudioMingw/g++.exe", "$StudioQt/bin/Qt6Quick.dll")) {
    if (!(Test-Path -LiteralPath $StudioFile)) { throw "Ferramenta ausente: $StudioFile. Execute scripts/setup.ps1." }
}
& $StudioCmake -S $StudioRoot -B "$StudioRoot/build" -G Ninja "-DCMAKE_MAKE_PROGRAM=$StudioNinja" "-DCMAKE_CXX_COMPILER=$StudioMingw/g++.exe" "-DCMAKE_PREFIX_PATH=$StudioQt" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'Configuração CMake falhou.' }
& $StudioCmake --build "$StudioRoot/build" --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Compilação falhou.' }
if ($Test) {
    & $StudioCtest --test-dir "$StudioRoot/build" --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Testes falharam.' }
}
