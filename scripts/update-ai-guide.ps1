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
$premiumIds = @(
    'premium:smartphone-premium-01',
    'premium:title-reveal-01',
    'premium:comparison-pack-01',
    'premium:dramatic-quote-01',
    'premium:lower-third-editorial-01',
    'premium:source-label-01',
    'premium:number-emphasis-01',
    'premium:question-hook-01',
    'premium:statistic-callout-pack-01'
)
$catalogPremiumIds = @($catalog.premiumAssets | ForEach-Object { $_.id })
if ($catalogPremiumIds.Count -ne 9 -or
    $catalog.productionCapabilities -notcontains 'premium-palette-overrides' -or
    $catalog.productionCapabilities -notcontains 'premium-factory-editorial-templates' -or
    $catalog.productionCapabilities -notcontains 'declarative-premium-templates' -or
    $catalog.productionCapabilities -notcontains 'premium-content-slot-overrides' -or
    $catalog.productionCapabilities -notcontains 'camera-path-assisted-framing' -or
    $catalog.productionCapabilities -notcontains 'camera-spatial-reveal' -or
    $catalog.productionCapabilities -notcontains 'camera-spatial-and-temporal-reveal') {
    throw 'O catálogo precisa refletir os nove premiums, slots via receita e os modos de enquadramento/revelação da câmera.'
}
foreach ($id in $premiumIds) {
    $asset = $catalog.premiumAssets | Where-Object { $_.id -eq $id } | Select-Object -First 1
    if ($null -eq $asset -or $asset.paletteTokens.Count -eq 0 -or $guideText -notmatch [regex]::Escape($id)) {
        throw "O guia ou o catálogo omitiu o premium registrado '$id' ou seus tokens de paleta."
    }
    if ($asset.slots.Count -eq 0 -or @($asset.slots | Where-Object { $_ -notmatch '^premium:' }).Count -gt 0 -or
        $asset.slotDefinitions.Count -eq 0 -or
        @($asset.slotDefinitions | Where-Object { -not $_.id -or -not $_.nodeId -or -not $_.kind }).Count -gt 0) {
        throw "O catálogo precisa separar papéis editáveis e definições reais de slots em '$id'."
    }
}
if ($guideText -notmatch 'premium-palette-v2.production.json' -or
    $guideText -notmatch 'camera-travelling-horizontal-v3.production.json' -or
    $guideText -notmatch 'camera-travelling-vertical-v3.production.json' -or
    $guideText -notmatch 'camera-path-spatial-v3.production.json' -or
    $guideText -notmatch 'camera-path-combined-v3.production.json' -or
    $guideText -notmatch 'premium-dramatic-quote-slots-v2.production.json' -or
    $guideText -notmatch 'premium-smartphone-slots-v2.production.json' -or
    $guideText -notmatch 'premium-question-hook-slots-v2.production.json' -or
    $guideText -notmatch 'premium-statistic-callout-slots-v2.production.json' -or
    $guideText -notmatch 'premium-factory-stage14-demo-v3.production.json' -or
    $guideText -notmatch 'definitionVersion') {
    throw 'O guia não inclui o contrato declarativo, os slots personalizados e os exemplos executáveis de câmera.'
}
