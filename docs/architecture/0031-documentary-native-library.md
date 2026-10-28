# ADR 0031 — ampliação do catálogo nativo documental

- Estado: Implementado; aparência padrão atualizada pela ADR 0035
- Data: 2026-10-26
- Branch: `feat/dynamic-documentary-library`

## Contexto

O catálogo inicial tinha apenas 14 itens e não oferecia diversidade suficiente para produções orientadas por IA com tipografia cinética, travelling, narrativa visual e infográficos. A biblioteca pessoal depende de importações manuais; é desejável fornecer um conjunto offline maior, acessível aos roteiros `studio-dark-production` v3 sem inventar caminhos ou `assetId`.

## Decisão

- Manter os 14 IDs anteriores e adicionar **58 IDs** (total de **72**) ao `nativeLibrary()`.
- Criar ilustrações vetoriais procedurais no compositor QPainter (`native_painter_extra.cpp`) e chamá-las pelo mesmo caminho de prévia/exportação dos outros objetos nativos.
- Incluir tecnologia, comunicação, gráficos, finanças, ciência e oito fundos documentais com tamanho base 1280×720.
- Registrar categorias novas no workspace e, em `WorkspaceStore::reload`, mesclar entradas nativas ausentes para instalações já existentes sem apagar itens pessoais, registros existentes ou exclusões do usuário.
- Manter `catalogVersion: 1` e receita v3: apenas IDs de ativos foram acrescentados; o formato do JSON permanece compatível.
- Gerar o guia técnico e capabilities a partir do registro real e publicar um guia criativo adicional de direção narrativa.

## Estado atual do catálogo

A lista de 72 IDs permanece. A aparência vetorial procedural descrita na decisão histórica foi substituída por PNG incorporado para 48 objetos e 12 cenários, mantendo seus IDs originais. `person` e os 11 gráficos/dados continuam procedurais. O desenho procedural dos outros 60 recursos ficou como fallback interno e não é mais uma opção visual separada. A implementação e a migração de bibliotecas existentes estão descritas na [ADR 0035](0035-ai-illustrated-library-variants.md).

## Considerações de qualidade

- Ilustrações `native` são simbólicas, úteis para infográficos, mas não equivalem a B-roll fotográfico.
- Painéis, porcentagens, mapas e gráficos do catálogo são **ilustrativos**, não obtêm dados nem validam fatos.
- Câmera continua sendo 2D com keyframes; profundidade só pode ser simulada pela composição de grupos.
- Presets de câmera não devem ser misturados com Visual Beats de câmera no mesmo shot.
- A ampliação usa um renderer dedicado, minimizando alterações no renderer legado; obras não dependem de mídia remota.

## Verificação necessária no Windows

1. `scripts/build.ps1 -Test`: compilar e verificar todos os IDs nativos.
2. `scripts/smoke.ps1`: validar prévia, composição e render.
3. `scripts/smoke-workspace.ps1`: conferir bibliotecas antigas, dados pessoais e migrações.
4. `scripts/smoke-production.ps1`: importar e renderizar `examples/documentario-dinamico-30s.production.json` com voz local pt-BR.
5. `scripts/update-ai-guide.ps1`: regenerar e conferir `docs/GUIA-PARA-IA.md` e `docs/capabilities-v1.json`.

**Estado de QA atualizado em 2026-10-28:** compilação incremental concluída; `production_recipe_audio_library` e `workspace_projects_library` passaram; `scripts/smoke.ps1`, `scripts/smoke-workspace.ps1` e `scripts/smoke-production.ps1` passaram com Microsoft Daniel. O teste visual local confirma os PNGs padrão e as prévias dos cenários. Benchmarks e testes GPU experimentais não foram executados nesta mudança.
