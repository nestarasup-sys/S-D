# ADR 0032 — fundação do motor gráfico 2.0

- Estado: aceita como direção arquitetural; Scene Graph segue experimental.
- Data: 26/10/2026

## Contexto

O documento do projeto já exigia medir `QPainter` e Qt Quick Scene Graph antes de decidir uma migração. O aplicativo usa `PreviewItem : QQuickPaintedItem`; seu `paint()` chama `CompositionPainter`. O render-worker também instancia `CompositionPainter` e produz `QImage` quadro a quadro antes de enviar RGBA ao FFmpeg. Isso mantém prévia, still e exportação no mesmo compositor, mas a prévia rasteriza a composição para uma imagem que o Qt Quick envia à textura. A avaliação de câmera e keyframes vem de `modules/core`; a composição resolve grupos, shots, direção de arte e mídia.

O Qt instalado é 6.8.3. Nesse release, trocar `QQuickPaintedItem` por um framebuffer acelerado não é uma opção portátil: o modo FBO desse item é ignorado em Qt 6.0–6.8. O Scene Graph tem suporte RHI a Direct3D 11, Vulkan, OpenGL e outros backends. A decisão precisa considerar qualidade, memória e exportação determinística, além do tempo por quadro. Referências: [QQuickPaintedItem 6.8](https://doc.qt.io/qt-6.8/qquickpainteditem.html), [Scene Graph 6.8](https://doc.qt.io/qt-6.8/qtquick-visualcanvas-scenegraph.html) e [QQuickRenderControl 6.8](https://doc.qt.io/qt-6.8/qquickrendercontrol.html).

## Diagnóstico atual

- `modules/core/src/project.cpp` avalia keyframes de objetos e câmera por índice de quadro; grupos e os IDs estáveis pertencem ao documento.
- `modules/composition/src/painter.cpp` calcula transformações hierárquicas e desenha fundos, shapes, texto/legendas, imagens, SVG, nativos e quadros de vídeo. Transições são compostas como camadas; o código é usado pela prévia e pelo worker.
- `modules/composition/src/native_painter*.cpp` implementa as ilustrações nativas como peças do mesmo passe QPainter. O catálogo não deve criar renderizadores paralelos por asset.
- `apps/desktop/preview_item.cpp` usa `QQuickPaintedItem`, transforma coordenadas de entrada e desenha a seleção como overlay da interface.
- `apps/render-worker/main.cpp` fixa o tempo no índice de quadro, compõe com `CompositionPainter`, envia RGBA ao FFmpeg e deixa áudio/mixagem no worker. A renderização não depende do relógio de parede.
- `modules/composition` publica `RenderSnapshot` por quadro. O QPainter e o protótipo GPU usam os mesmos estados resolvidos para câmera, grupos, keyframes, visibilidade, opacidade, direção de arte e shots.

## Medição comparativa

Executado em 26/10/2026 no Windows, Qt 6.8.3, AMD Radeon RX 7600 / driver 32.0.32015.7029 e AMD Ryzen 5 4600G. Cada perfil renderizou 60 quadros de um único projeto sintético com 120 formas arredondadas agrupadas, keyframes de grupo/objeto e movimento de câmera; há três repetições aquecidas por resolução. CPU é `CompositionPainter` para `QImage`; GPU é um protótipo `QQuickItem`/`QSGGeometryNode` em Direct3D 11, com `swapInterval=0` solicitado antes da criação dos recursos. Tempos incluem a avaliação/preparação das geometrias em cada backend, sem limitar o Scene Graph à taxa de atualização do monitor.

Esta tabela é a medição exploratória da etapa 1. O comparador de PSNR daquele protótipo não normalizava todos os formatos de canal das `QImage`; portanto, os valores de PSNR abaixo são apenas históricos e foram substituídos pelas métricas normalizadas de cor/forma/borda documentadas na etapa 3.

| Canvas | QPainter CPU ms/quadro | Scene Graph ms/quadro | PSNR CPU × protótipo | RSS do processo CPU → Scene Graph |
| --- | ---: | ---: | ---: | ---: |
| 1280×720 | 25,71 | 7,19 | 24,09 dB | 25,9 → 60,8 MiB |
| 1920×1080 | 36,30 | 6,27 | 24,13 dB | 30,3 → 73,9 MiB |
| 1080×1920 | 36,60 | 5,99 | 24,13 dB | 30,3 → 74,0 MiB |

O tempo por quadro do Scene Graph foi 72% menor em 720p, 83% menor em Full HD e 84% menor em retrato neste conjunto limitado. O RSS do processo ficou 2,3–2,4× maior no Scene Graph; esse número inclui a imagem CPU de referência ainda residente e não mede a VRAM dedicada do driver. A estimativa separada soma apenas geometria vetorial e um framebuffer RGBA, não é uma medição da memória total da GPU. A captura do protótipo tem diferença visual perceptível: 24 dB serve somente como guarda de regressão grosseira deste experimento e não atinge a consistência observada no pipeline atual (44,09 dB no smoke de preview/export). O benchmark não cobre texto, fontes, imagens, SVG, nativos, vídeo, efeitos, transições nem a exportação fora da tela; não prevê o desempenho de um motor completo. Os três tempos CPU tiveram dispersão considerável entre repetições; os valores da tabela são medianas, e o relatório conserva cada tentativa.

Os arquivos completos, PNGs e métricas estão em `artifacts/renderer-benchmark-20261026-032146512/` (fora do Git). O procedimento reproduzível é `scripts/benchmark-renderers.ps1 -Frames 60 -Nodes 120 -Repeats 3`.

## Decisão

1. Manter o projeto existente como fonte canônica. Tempo é sempre `{frame, fps}`; não usar timers de animação da interface como fonte para o render final.
2. Evoluir para um avaliador compartilhado que gera um `RenderSnapshot` imutável por quadro: transformações hierárquicas, opacidade/visibilidade, câmera, shot, direção de arte, ordem, bounds e referências a recursos resolvidos.
3. Definir um contrato de comandos/grupos de renderização derivado desse snapshot. Formas vetoriais, assets nativos e premium, tipografia, efeitos, profundidade 2.5D e mídia real são operações do mesmo grafo e compartilham tempo, coordenadas, câmera e transformações.
4. Ter adaptadores de backend, não motores por categoria: `RasterBackend` mantém QPainter e garante exportação/compatibilidade; `SceneGraphBackend` traduz o mesmo snapshot para nós/materials RHI na prévia; uma futura exportação GPU só é aceita quando sua saída fora de tela passar os mesmos testes determinísticos e de paridade.
5. Centralizar tipografia/fontes, geometria vetorial, texturas e decode de mídia em serviços/cache com chaves por revisão e orçamentos explícitos. Nativos e premium descrevem parâmetros/peças reutilizáveis; não implementam loops de composição isolados.
6. Manter seleção, alças e overlays de inspeção somente na UI. Eles não entram no snapshot exportado.
7. Não alterar schemas JSON, receitas, sincronização de voz ou projetos existentes nesta etapa. Nenhum asset premium é incluído por este protótipo.

```mermaid
flowchart LR
  P[Projeto e timeline] --> E[Avaliador compartilhado por frame]
  T[Tempo: frame e fps] --> E
  C[Câmera e hierarquia] --> E
  L[Biblioteca e mídia] --> R[Resolver de recursos]
  E --> S[RenderSnapshot / grafo de comandos]
  R --> S
  S --> Q[RasterBackend QPainter]
  S --> G[SceneGraphBackend Qt Quick / RHI]
  Q --> X[Preview, still e export atual]
  G --> V[Preview GPU experimental]
```

## Primeira melhoria entregue

`RenderSnapshot` agora é construído para um quadro e possui cópias próprias do tempo/FPS/canvas, câmera resolvida, shots/transição, nós em ordem de camada, hierarquia, bounds, keyframes avaliados, opacidade/visibilidade, direção de arte e referências de mídia. O `CompositionPainter` consome os estados resolvidos e conserva a sequência de operações QPainter para manter a rasterização existente. O snapshot não mantém ponteiros para o `Project` editável e expõe acesso constante ao estado publicado. A representação define o contrato para um consumidor Scene Graph futuro; nenhum backend GPU foi ligado ao editor.

As imagens de referência cobrem o projeto demo, um quadro misto com PNG/SVG/vídeo, nativos e palavras de legenda, e uma transição dissolve. O comparador normaliza os formatos de canal antes do cálculo: os três resultados tiveram PSNR de 99 dB. Testes também verificam hierarquia, opacidade herdada, limites, câmera, referências de mídia, shots e quadro de saída da transição. O catálogo tem 72 nativos. Não houve mudança de capacidade JSON: receitas v1/v2/v3, schemas e guia da IA permanecem iguais.

As rodadas acima são registros históricos da extração do snapshot. A inspeção posterior identificou que a comparação visual antiga dependia de formato de canal não normalizado e de cor de vértice não premultiplicada; por isso, aqueles ~24 dB não devem ser comparados diretamente com o protocolo da etapa 3. O protótipo segue isolado e não autoriza adoção no editor.

O protótipo em `apps/scenegraph-prototype/` consome `RenderSnapshot::primary()`; não reavalia câmera, keyframes, hierarquia, opacidade herdada ou shots. Ele converte os nós Shape em uma malha QSG compartilhada, mantendo `drawOrder`. O material usa cores RGBA premultiplicadas, blending explícito e uma franja alfa de um pixel para antialias. Uma suíte sintética exercita retângulo, retângulo arredondado, elipse, contorno retangular, polilinha e caminho convexo; gera PNGs de referência QPainter por classe e imagens de diferença amplificadas. O item tem clipping pelo limite do canvas.

Essas primitivas são contratos internos do renderizador; receitas JSON ainda não escolhem o tipo de forma e o catálogo nativo não mudou. O Scene Graph continua experimental e não está conectado ao editor nem à exportação. `QSGVertexColorMaterial` não produziu geometria útil no backend software do Qt 6.8; o relatório marca esse backend como incompatível. Na entrega da etapa 3, texto/tipografia, assets nativos, imagens e SVG ainda estavam fora do protótipo; a etapa 4 registra a composição posterior desses conteúdos via texturas rasterizadas em CPU. Vídeo e composição de shots/transições continuam fora do protótipo.

O benchmark executável separa tempo total QPainter, construção do snapshot, preparação da malha, submissão/espera do `frameSwapped` e captura da janela. Mede RAM residente do processo e bytes reais da malha do último quadro; a VRAM dedicada não é medida. `frameSwapped` não é uma medição isolada de execução GPU. `scripts/benchmark-renderers.ps1` padrão cobre 20, 120 e 200 elementos, grupos aninhados, câmera animada, opacidade e objetos parcialmente fora do canvas, com três repetições por resolução; os resultados e diferenças ficam em `artifacts/renderer-benchmark-*`.

## Etapa 3 — primitivas vetoriais no Scene Graph

O Scene Graph agora consome somente `RenderSnapshot::primary()`: não reavalia keyframes, hierarquia, câmera, opacidade herdada, visibilidade, ordenação ou estado de shot. O snapshot fornece as transformações resolvidas; seus Shapes são traduzidos em malha de triângulos QSG. A prévia principal e a exportação continuam no `CompositionPainter`/QPainter.

O protótipo oferece retângulos, retângulos arredondados, elipses, contornos retangulares, linhas e caminhos convexos preenchidos ou traçados. O passe aplica transformação de grupo e câmera, opacidade, ordem do snapshot, clipping ao item e franja alfa de um pixel. A cor dos vértices é premultiplicada antes do blending. Os lotes são divididos antes de 60.000 vértices: a primeira matriz completa revelou que o caminho D3D11 do Qt descartava triângulos ao ultrapassar seu limite de índices de 16 bits numa única geometria (200 formas). A divisão em lotes preserva a ordem de desenho e resolveu a perda.

O comparador normaliza as imagens para ARGB32. PSNR fica como indicador geral e diagnóstico; os bloqueios de regressão também medem IoU de primeiro plano, F1 de bordas com tolerância de um pixel e MAE de cor. Para cenas preenchidas, o critério é IoU ≥ 0,94, edge F1 ≥ 0,95 e MAE ≤ 3. Para contornos e linhas de 3 px, IoU ≥ 0,74 é permitido devido à cobertura subpixel distinta entre rasterizadores; o critério continua exigindo edge F1 ≥ 0,95 e MAE ≤ 3. Os limites vêm acompanhados por PNGs dourados de QPainter e imagens de diferença amplificadas; defeitos como geometria ausente, bordas deslocadas acima de 1 px ou erro médio de cor acima de 3 falham independentemente do PSNR.

Benchmark completo executado em 26/10/2026 no Windows, Qt 6.8.3, Direct3D11. Cada cenário renderizou 60 quadros em três execuções. A cena tem grupos aninhados, câmera e transformações animadas, opacidade e elementos parcialmente fora do canvas. Os tempos estão em ms/quadro, exceto captura (ms por imagem final); RAM é o working set do processo medido durante o cenário. A coluna de espera inclui submissão e `frameSwapped`, não representa tempo puro de GPU.

| Canvas | Formas | QPainter | Snapshot | Preparação Scene Graph | Submissão/espera | Captura | RAM CPU | RAM processo Scene Graph | Geometria |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1280×720 | 20 | 4,79 | 0,14 | 0,38 | 5,50 | 3,40 | 25,3 MiB | 63,1 MiB | 110 KiB |
| 1280×720 | 120 | 7,04 | 0,56 | 1,75 | 5,74 | 3,96 | 26,0 MiB | 74,0 MiB | 660 KiB |
| 1280×720 | 200 | 8,83 | 0,90 | 2,76 | 6,00 | 3,72 | 26,3 MiB | 73,2 MiB | 1.097 KiB |
| 1920×1080 | 20 | 9,11 | 0,14 | 0,39 | 6,09 | 6,33 | 29,7 MiB | 76,2 MiB | 110 KiB |
| 1920×1080 | 120 | 13,02 | 0,56 | 1,74 | 6,07 | 6,62 | 30,4 MiB | 87,1 MiB | 660 KiB |
| 1920×1080 | 200 | 13,53 | 0,95 | 2,75 | 5,82 | 6,66 | 30,8 MiB | 86,1 MiB | 1.097 KiB |
| 1080×1920 | 20 | 9,30 | 0,15 | 0,39 | 6,03 | 6,50 | 29,7 MiB | 76,4 MiB | 110 KiB |
| 1080×1920 | 120 | 13,84 | 0,59 | 1,84 | 5,90 | 6,60 | 30,4 MiB | 87,4 MiB | 660 KiB |
| 1080×1920 | 200 | 15,55 | 0,95 | 2,81 | 6,09 | 6,50 | 30,8 MiB | 86,5 MiB | 1.097 KiB |

A matriz não indica que o Scene Graph seja universalmente mais rápido: a preparação somada à submissão/espera consome cerca de 5,9–6,1 ms/quadro neste protótipo, e a captura acrescenta 3,4–6,7 ms por imagem. A RAM residente do processo Scene Graph fica 37–57 MiB acima da referência CPU; inclui Qt Quick e a imagem CPU mantida para comparação. VRAM dedicada não foi mensurada com confiabilidade. O desvio-padrão do QPainter variou de 0,23 a 2,42 ms/quadro; preparação do Scene Graph ficou entre 0,01 e 0,07 ms/quadro. O tempo de espera não isola execução GPU.

Na suíte mista de 120 formas (20 de cada classe) em 1280×720: PSNR 27,85 dB, IoU 0,948, edge F1 0,999 e MAE 1,96. As seis classes passaram as imagens de referência. Resumo por classe em 1280×720 / 20 formas:

| Primitiva | PSNR | IoU | Edge F1 | MAE |
| --- | ---: | ---: | ---: | ---: |
| Retângulo arredondado | 31,68 dB | 0,987 | 1,000 | 0,93 |
| Retângulo | 31,50 dB | 0,987 | 1,000 | 0,96 |
| Elipse | 32,74 dB | 0,987 | 1,000 | 0,74 |
| Contorno retangular | 30,13 dB | 0,775 | 1,000 | 1,17 |
| Linha | 32,13 dB | 0,769 | 0,998 | 0,75 |
| Caminho convexo | 32,45 dB | 0,985 | 1,000 | 0,76 |

IoU menor em linhas/contornos vem de diferenças na cobertura de traços finos de 3 px, não de perda do percurso; edge F1 e MAE confirmam proximidade das bordas e das cores. O relatório completo, PNGs dourados e diferenças está em `artifacts/renderer-benchmark-20261026-044152515/`; inclui dispersão por execução e comparação nos três canvases. O probe identificou que `QSGVertexColorMaterial` não entrega geometria útil ao backend software Qt 6.8, então esse backend está explicitamente não suportado pelo protótipo.

## Plano incremental de migração

| Corte | Trabalho | Critério de saída |
| --- | --- | --- |
| 0 — Instrumentação (feito parcialmente) | Benchmark reproduzível, teste de cache SVG, fotografia de referência e limites de memória. | Cenários de canvas e relatórios repetíveis; paridade atual preview/export protegida por smoke. |
| 1 — Avaliação compartilhada (implementado nesta etapa) | Extrair hierarquia, estado resolvido, câmera, ordem e metadados de shots para `RenderSnapshot`; QPainter consome estados resolvidos sem alterar pixels/schema. | Imagens de referência idênticas; a mesma `CompositionPainter` atende prévia, still e worker. |
| 2 — Primitivas vetoriais RHI (parcial, protótipo isolado) | QSG desenha retângulos, retângulos arredondados, elipses, contornos, linhas e caminhos convexos a partir do `RenderSnapshot`; aplica transformações, opacidade, ordem e clip do canvas. | O material não funciona no backend software; primitiva de curva Bézier, paths côncavos, clipping arbitrário e Scene Graph no editor continuam pendentes. Benchmarks não demonstram ganho geral nem medem VRAM. |
| 3 — Texto e assets nativos (composição experimental implementada; rasterização CPU) | Rasterizar camadas com `CompositionPainter` e compartilhá-las como texturas no Scene Graph; conversão dos nativos para geometria GPU ainda pendente. | Fontes/layout existentes, 72 nativos e compatibilidade visual; não implica vetorização GPU. |
| 4 — Imagens e SVG (composição experimental implementada; rasterização CPU) | Abrir imagem/SVG pelos caminhos existentes e compor textura versionada com LRU limitado; efeitos ainda pendentes. | Limites de memória, invalidação por mudança do recurso e métricas de cor/alpha. |
| 5 — 2.5D e vídeo | Planos com profundidade e câmera compartilhada; serviço de decode com timestamp/seek, frame cache e fallback CPU. | Preview, still e MP4 comparados em saltos, trim, speed, shots e áudio embutido. |
| 6 — Exportação RHI opcional | Avaliar `QQuickRenderControl`/target RHI para batches, sob controle de quadro e dispositivo. | Exatidão temporal, cor, fontes e estabilidade do worker validadas antes de adoção. |

Cada etapa mantém o rasterizador funcional. Só considerar troca ou backend selecionável após paridade visual definida por conteúdo, memória aceitável no hardware mínimo, tempo de export e fallback reproduzíveis. O benchmark inicial não aprova uma substituição.

## Verificação desta decisão

- `scripts/build.ps1 -Test`: 5/5 testes passaram, incluindo o novo teste de snapshot e regressão visual.
- `scripts/smoke.ps1`: passou; preview/export 44,09 dB em 1280×720.
- `scripts/smoke-workspace.ps1`: passou; projetos locais, catálogo completo e 72 recursos nativos.
- `scripts/smoke-production.ps1`: passou com Microsoft Daniel; inclui receita, áudio, shots, beats, retrato, vídeo e comparações preview/export. Maior caso: 2700 quadros, 18 shots e 20 Visual Beats; preview/export 42,50 dB.
- `scripts/benchmark-renderers.ps1`: matriz padrão completa (20/120/200 formas × três canvases × três repetições), três suítes mistas e seis referências de classe; passou em Direct3D11. Uma falha inicial em 200 formas levou à divisão da geometria em lotes; a execução completa passou depois da correção. O backend software foi reportado como não suportado. Resultados e imagens: `artifacts/renderer-benchmark-20261026-044152515/`.

## Etapa 4 — camadas híbridas de texto, mídia e biblioteca

O protótipo agora intercala primitivas vetoriais GPU e texturas no mesmo `QSGNode` tree, na ordem de `RenderSnapshot::primary()`. A avaliação de câmera, grupos, keyframes, opacidade, visibilidade e draw order continua compartilhada. Para texturas, o protótipo aplica a transformação resolvida e a câmera no nó de cena; a imagem é rasterizada em coordenadas locais por `CompositionPainter::rasterNodeLayer`, evitando reimplementar layout de texto e desenho dos nativos. A camada do fundo também reutiliza `CompositionPainter::rasterBackgroundLayer` quando selecionada.

Texto usa o mesmo painter e subsistema de fontes/layout do compositor de produção: fontes do sistema e fontes externas cadastradas, quebra, alinhamento, rotação/escala via transformação do nó, opacidade herdada e texto de legenda com palavra ativa por quadro. Fontes externas foram verificadas com Arial carregada de arquivo e a cena híbrida permaneceu dentro dos critérios visuais. Todos os 72 IDs de `nativeLibrary()` são exercitados pelo atlas; variantes dinâmicas incluem o frame na chave do cache. Isso comprova composição dos assets pelo Scene Graph, mas suas formas continuam desenhadas em CPU pelo painter nativo e transferidas como textura. Não é conversão dos 72 assets em geometria GPU. PNG transparente, JPEG e SVG foram lidos pelo mesmo `QImageReader`/cache SVG e transformados em texturas. WebP é usado quando o plugin de leitura/escrita correspondente está instalado; neste runtime Qt 6.8 a lista de formatos não contém WebP e o writer não gera a fixture, então o caso é explicitamente reportado como indisponível, sem mascarar falha dos formatos suportados. Não há decode de vídeo nesta etapa.

O cache de textura é LRU e limitado a 64 MiB (4 MiB no cenário de pressão). A chave inclui conteúdo visual, dimensão, metadados de arquivo e, para conteúdo animado, o frame. Camadas sem mudança são reutilizadas; chaves alteradas são invalidadas. O Scene Graph cria, usa e libera `QSGTexture` apenas na thread de renderização; a limpeza após invalidação da cena é agendada nessa thread. Imagens que excedem o limite podem ser apresentadas sem cache, sem estourar o orçamento persistente. O raster CPU ocorre no ciclo de atualização do Scene Graph e seu custo é medido separadamente. O cronômetro de `createTextureFromImage` mede submissão/preparação CPU, não conclusão isolada do upload físico/VRAM.

As regressões adicionadas incluem golden images híbridas estática e animada e atlas de nativos, comparação de PSNR, MAE RGB/alpha, IoU de primeiro plano e F1 de bordas em até 1 px, contagem de camadas transparentes, estabilidade/invalidação de chaves e pressão de cache. A referência híbrida inclui forma direta GPU, título grande com quebra, legenda sincronizada por palavra, nativo animado, PNG alfa, JPEG e SVG; a imagem WebP é incluída quando o runtime tem os plugins Qt necessários, indisponíveis no ambiente medido. A matriz híbrida mediu: 1280×720 animada PSNR 31,84 dB, IoU 0,9968, edge F1 0,9973, MAE 0,605 e alpha MAE 0; estática PSNR 32,67 dB, IoU 0,9966, edge F1 0,9999, MAE 0,540. Atlas de 72 nativos em 2×: PSNR 33,93 dB, IoU 0,9760, edge F1 0,9156 e MAE 1,416. Para o atlas, o bloqueio de borda usa F1 ≥ 0,91, definido após comparar raster 1× (F1 0,9048), 2× (0,9156) e 4× (0,8975, com mais uso de RAM/evicções); o limite é específico desse atlas e não é apresentado como igualdade pixel a pixel. A cena híbrida usa o limite mais estrito de edge F1 ≥ 0,95. Foram salvas referências em `tests/reference/render-hybrid/`.

O orçamento padrão de cache persistente é 64 MiB; no ensaio de pressão da cena animada em 720p, o orçamento de 4 MiB foi respeitado (pico de 3,8 MiB), houve 63 evicções, e as métricas visuais permaneceram iguais às da cena sem pressão. Isso valida limite e política de descarte para esse cenário, não prevê o working set de projetos arbitrários. A RAM do processo também inclui Qt, imagens de comparação e alocações do driver; VRAM dedicada não é medida com confiabilidade. Para benchmark, raster CPU e chamadas de submissão de textura são relatados separadamente; `frameSwapped` inclui sincronização e não é duração pura de shader/render GPU.

Ainda não suportados no protótipo: quadros de vídeo, transições de shots, efeitos premium, paths arbitrários/côncavos e backend Qt Quick Software. O editor principal e exportação FFmpeg continuam no QPainter. Nenhum campo JSON/receita ou capacidade de IA mudou; não regenerar o guia/catálogo é intencional e o contrato v1/v2/v3 permaneceu igual.

Benchmark completo da etapa 4 executado em 26/10/2026, Direct3D11, Qt 6.8.3, AMD Radeon RX 7600 e Ryzen 5 4600G. Cada cenário da matriz de formas usou 60 quadros e três repetições. Cenas híbridas estáticas/animadas também usaram 60 quadros e três repetições; o atlas foi executado três vezes, e o caso de pressão uma vez. Valores de tempos são medianas em ms/quadro; RAM é working set amostrado do processo, não VRAM. QPainter rasteriza a referência e o conteúdo de camada. “espera” mede submissão/espera por `frameSwapped`, não execução isolada da GPU.

| Cena híbrida | Canvas | QPainter CPU ms/quadro | Rasterização CPU ms/quadro | Submissão textura ms/quadro | Preparação Scene Graph ms/quadro | Submissão/espera ms/quadro | Captura ms | RAM QPainter | RAM Scene Graph | Cache pico |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Animada | 1280×720 | 5,58 | 0,89 | 0,01 | 1,40 | 6,58 | 3,85 | 32,2 MiB | 87,8 MiB | 63,3 MiB |
| Estática | 1280×720 | 4,22 | 0,00¹ | 0,00¹ | 0,44 | 5,18 | 3,26 | 30,8 MiB | 75,2 MiB | 9,2 MiB |
| Animada | 1920×1080 | 11,06 | 2,05 | 0,01 | 2,58 | 7,02 | 6,37 | 35,6 MiB | 111,5 MiB | 62,5 MiB |
| Estática | 1920×1080 | 8,95 | 0,00¹ | 0,00¹ | 0,44 | 5,02 | 5,99 | 35,3 MiB | 102,4 MiB | 20,8 MiB |
| Animada | 1080×1920 | 9,50 | 1,64 | 0,01 | 2,18 | 6,79 | 6,30 | 36,3 MiB | 112,2 MiB | 62,5 MiB |
| Estática | 1080×1920 | 7,69 | 0,00¹ | 0,00¹ | 0,43 | 5,04 | 6,72 | 35,2 MiB | 102,5 MiB | 20,8 MiB |
| Atlas de 72 nativos | 1280×720 | 26,95 | 0,17 | 0,00¹ | 0,63 | 6,79 | 3,56 | 31,4 MiB | 108,2 MiB | 36,8 MiB |
| Cache reduzido, 4 MiB | 1280×720 | 5,62 | 3,59 | 0,01 | 4,21 | 7,05 | 4,14 | 31,8 MiB | 96,7 MiB | 3,8 MiB |

¹ Sem upload/rasterização nova nos quadros medidos, porque as camadas permaneceram no cache após o aquecimento. O ensaio animado atualiza conteúdo dependente do quadro. O atlas toca os 72 IDs; as ilustrações continuam rasterizadas por QPainter.

| Cena híbrida | Canvas | PSNR | IoU | F1 de bordas | MAE RGB | MAE alpha |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Animada | 1280×720 | 31,84 dB | 0,9968 | 0,9973 | 0,605 | 0 |
| Estática | 1280×720 | 32,67 dB | 0,9966 | 0,9999 | 0,540 | 0 |
| Animada | 1920×1080 | 36,78 dB | 0,9981 | 0,9986 | 0,310 | 0 |
| Estática | 1920×1080 | 36,34 dB | 0,9987 | 0,9998 | 0,287 | 0 |
| Animada | 1080×1920 | 36,44 dB | 0,9982 | 0,9999 | 0,307 | 0 |
| Estática | 1080×1920 | 36,21 dB | 0,9984 | 1,0000 | 0,293 | 0 |
| Atlas de 72 nativos | 1280×720 | 33,93 dB | 0,9760 | 0,9156 | 1,416 | 0 |
| Cache reduzido, 4 MiB | 1280×720 | 31,84 dB | 0,9968 | 0,9973 | 0,605 | 0 |

As seis cenas híbridas passaram os limites: IoU ≥ 0,94, F1 de bordas ≥ 0,95 e MAE RGB ≤ 3; alpha MAE foi zero. O atlas passou com limite de bordas específico F1 ≥ 0,91, fundamentado pelo sweep de 1×/2×/4× descrito acima. Sob cache de 4 MiB, as métricas foram iguais à híbrida animada padrão de 720p dentro da precisão reportada. Fonte externa Arial carregada de `C:\Windows\Fonts\arial.ttf` também foi validada separadamente: PSNR 31,39 dB, IoU 0,9970, edge F1 0,9987, MAE 0,674 e alpha MAE 0. O teste padrão confirma fontes do sistema; as opções de fonte seguem o QPainter atual.

Na matriz geométrica de 20/120/200 formas, a RAM mediana do processo Scene Graph ficou entre 71,3 e 99,3 MiB, contra 25,7–31,2 MiB do caminho QPainter de referência. O benchmark não prova que GPU seja mais rápida em todas as cenas; no híbrido, as camadas dinâmicas mantêm custos de rasterização em CPU e a espera por quadro é maior que a preparação CPU do grafo. O backend Software foi classificado como não suportado: `QSGVertexColorMaterial` não produziu geometria utilizável no probe. VRAM dedicada não foi mensurada. O relatório detalhado, provas e imagens de diferença estão em `artifacts/renderer-benchmark-20261026-051952438/`; reprodução com `scripts/benchmark-renderers.ps1`.
