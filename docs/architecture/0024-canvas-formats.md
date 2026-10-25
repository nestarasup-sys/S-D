# ADR 0024 — formatos de tela paisagem e retrato

- Estado: Aceita
- Data: 2026-10-25

## Contexto

O projeto já persiste largura e altura, e prévia/render compartilhavam a composição, mas as receitas e o fluxo de novo projeto assumiam 1280×720. A produção vertical precisa escolher o quadro antes de resolver câmera, legendas e elementos, sem migrar ou distorcer projetos antigos.

## Decisão

- Manter 1280×720 como padrão em receitas existentes e em receitas v3 que omitam `canvas`.
- Aceitar `canvas: { width, height }` apenas em receita v3, com inteiros e pares exatos 1280×720, 1920×1080 ou 1080×1920. As coordenadas da composição são pixels desse quadro.
- A compilação transfere as dimensões ao projeto editável. Câmeras omitidas começam no centro correspondente; legendas e fundos são posicionados usando a altura/largura efetiva.
- A tela de novo projeto oferece os mesmos três formatos. O modelo de projeto e o render já suportam 1080×1920, portanto não há nova migração de schema.
- Cenários nativos mantêm a arte base 1280×720 e usam `cover` central para preencher canvases de proporção diferente, preservando a escala e recortando as laterais quando necessário.
- O preview, still e exportação continuam usando o compositor compartilhado para manter os mesmos pixels e o mesmo timing.

## Consequências

- Receitas antigas permanecem no formato e dimensões originais.
- Receitas v3 podem dirigir conteúdo para plataformas verticais; o guia e catálogo anunciam apenas as resoluções aceitas.
- Elementos e textos de uma receita vertical devem usar coordenadas e tamanhos pensados para o quadro retrato. A validação não reposiciona automaticamente conteúdo fora do canvas.
- Resoluções além dos três presets continuam rejeitadas até serem avaliadas e verificadas em produção.

## Verificação

O teste de produção valida parse, dimensões, câmera padrão, legendas, projeto e composição vertical; o teste de workspace cria e reabre um projeto 1080×1920. O smoke de produção sintetiza voz pt-BR, renderiza o exemplo vertical, confere dimensões/quadros do MP4, compara um frame exportado com o still do compositor e abre o projeto na interface.
