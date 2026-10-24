. "$PSScriptRoot/environment.ps1"
$StudioFormatter = Join-Path $StudioRoot '.tools/formatter/clang_format/data/bin/clang-format.exe'
if (!(Test-Path -LiteralPath $StudioFormatter)) { throw 'Execute scripts/setup.ps1 para preparar clang-format.' }
$StudioSources = Get-ChildItem -LiteralPath "$StudioRoot/apps", "$StudioRoot/modules", "$StudioRoot/tests" -Recurse -File | Where-Object { $_.Extension -in '.cpp', '.hpp' }
foreach ($StudioSource in $StudioSources) {
    & $StudioFormatter -i $StudioSource.FullName
    if ($LASTEXITCODE -ne 0) { throw "Falha ao formatar $($StudioSource.FullName)." }
}
