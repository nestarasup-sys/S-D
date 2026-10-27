. "$PSScriptRoot/environment.ps1"
& "$StudioBin/studio_produce.exe" --guide "$StudioRoot/docs/GUIA-PARA-IA.md"
if ($LASTEXITCODE -ne 0) { throw 'Falha ao gerar guia.' }
& python "$StudioRoot/scripts/embed-ai-production-example.py" --guide "$StudioRoot/docs/GUIA-PARA-IA.md" --example "$StudioRoot/examples/carros-caros-directors-cut-90s.production.json"
if ($LASTEXITCODE -ne 0) { throw 'Falha ao inserir o exemplo profissional testado no guia.' }
& "$StudioBin/studio_produce.exe" --capabilities "$StudioRoot/docs/capabilities-v1.json"
if ($LASTEXITCODE -ne 0) { throw 'Falha ao gerar catalogo.' }
$guideText = Get-Content -LiteralPath "$StudioRoot/docs/GUIA-PARA-IA.md" -Raw
$catalog = Get-Content -LiteralPath "$StudioRoot/docs/capabilities-v1.json" -Raw | ConvertFrom-Json
if ($guideText -notmatch 'opacity.*inicial seja 0' -or
    $guideText -notmatch 'revelação sequencial de três objetos' -or
    $guideText -match '200 objetos|200 nós|no máximo 200') {
    throw 'O guia gerado não corresponde à semântica atual dos Visual Beats ou contém o antigo limite de objetos.'
}
if ($null -ne $catalog.maxNodes -or $catalog.nodeLimitPolicy -ne 'resource-bound' -or
    $catalog.productionCapabilities -notcontains 'instant-visual-beat-show-hide') {
    throw 'O catálogo de capacidades não corresponde ao limite baseado em recursos e ao Visual Beat atualizado.'
}
