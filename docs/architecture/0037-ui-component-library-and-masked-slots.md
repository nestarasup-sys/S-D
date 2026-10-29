# ADR 0037: Biblioteca adicional de componentes UI com slots mascarados

- Status: accepted
- Date: 2026-10-28

## Context

O pacote `studio-dark-ui-66-v1-CODEX` fornece 66 PNGs em seis grupos e declara slots de texto e mídia. Vinte e uma aberturas possuem máscaras em tons de cinza. O usuário solicitou uma expansão da biblioteca, sem substituir os nativos, o apresentador, os gráficos dinâmicos ou os assets anteriores.

## Decision

- Registrar os 66 itens sob seus IDs originais `ui:*`, lidos do manifesto incorporado. Não há 66 construtores C++ nem variantes duplicadas.
- Embutir manifesto, artes e máscaras PNG em recursos Qt. A biblioteca mostra as artes originais como miniaturas e separa os componentes em seis categorias.
- Expandir cada referência `kind: "library"` para um grupo e nós comuns. A arte PNG fica acima do conteúdo interno. Texto/imagem/vídeo do slot são nós editáveis normais e usam a avaliação de hierarquia, câmera, keyframes, tempo e opacidade já existente.
- Persistir `mask.source` e o recorte normalizado da máscara no schema de projeto 17. O QPainter rasteriza o conteúdo do slot numa superfície local limitada, converte luminância em alfa e aplica `DestinationIn` antes de compor o resultado na ordem de camadas. Imagens cobrem o recorte com escala preenchida; vídeos usam a mesma máscara após decodificação local.
- Expor somente tipos executáveis: `overlay-text` (texto), `overlay-content` (texto/imagem/vídeo) e `masked-content` (texto/imagem/vídeo). O `overlay-rotatable-needle` de `ui:pressure-gauge` é sugestão do manifesto, não um slot funcional.
- Aceitar caminhos de mídia locais relativos à pasta do JSON da receita e rejeitar URL, caminho absoluto e traversal. Na importação, conteúdo de slots é copiado por hash para o armazenamento gerenciado do workspace. O projeto continua dependendo daquele workspace local e não é um pacote portátil entre computadores.
- Preservar os pixels PNG. Transformações externas de nó são suportadas; recoloração por `palette` ou animação interna da arte raster não são anunciadas.

## Consequences

- Receitas v2 podem preencher slots de texto/imagem e v3 também pode preencher slots de vídeo. O parser valida os slots reais do manifesto e inclui o ID do componente/slot no erro.
- O inspetor permite editar texto, substituir imagem ou vídeo em slots mascarados e manter a máscara do componente.
- Prévia, salvamento/reabertura e exportação usam os mesmos nós/máscaras no QPainter. O motor GPU experimental não é necessário.
- As superfícies temporárias de máscara são limitadas em tamanho; se a alocação falhar, o conteúdo do slot é omitido e não vaza para fora do recorte.
- O catálogo inicial passa a 147 itens ativos: 72 nativos, nove premium e 66 componentes UI, em 17 categorias. Dados pessoais e exclusões são preservados pelo `WorkspaceStore`.

## Validation

`production_recipe_audio_library` valida registro, exemplo JSON, slots de texto/imagem/vídeo, compilação, schema 17, renderização QPainter, persistência e rejeição de slots desconhecidos. `workspace_projects_library` valida os 66 IDs, seis categorias, miniaturas, inserção e salvamento/reabertura. `scripts/smoke-ui-components.ps1` executa uma produção curta com imagem e vídeo mascarados, abre o projeto no editor, exporta MP4 e compara um quadro com o QPainter. `scripts/smoke-workspace.ps1` verifica a interface da biblioteca. Benchmarks longos e testes GPU ficam fora do escopo.
