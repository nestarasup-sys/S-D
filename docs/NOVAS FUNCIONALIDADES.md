# PROJETO: EVOLUÇÃO DO STUDIO DARK — MOTOR DE EDIÇÃO AUDIOVISUAL DIRIGIDO POR IA

## ANDAMENTO DA IMPLEMENTAÇÃO — 26/10/2026

**Fases 1–4 do checklist original: concluídas. Plano global: em andamento.** A cor independente e os presets reutilizáveis de títulos/textos agora estão implementados; outras evoluções ainda listadas no documento continuam pendentes.

Este quadro separa as funcionalidades implementadas, as limitações atuais e as próximas evoluções sugeridas. O checklist principal das quatro fases está concluído.

### Entrega implementada: fundação e benchmark do motor gráfico 2.0

- O diagnóstico confirmou um compositor `CompositionPainter` compartilhado por prévia, still e exportação; o render-worker fixa o tempo por frame e usa o mesmo avaliador de keyframes/câmera. A prévia atualmente rasteriza via `QQuickPaintedItem`/`QPainter`.
- Foi criado um protótipo independente `apps/scenegraph-prototype/` que envia formas com grupos, keyframes e câmera ao Qt Quick Scene Graph. Não foi ligado ao editor nem substituiu o backend raster.
- Benchmark aquecido, 3 repetições de 60 quadros e 120 formas, Qt 6.8.3/Direct3D11, Radeon RX 7600 e Ryzen 5 4600G: 1280×720 mediana de 25,71 ms/quadro no QPainter e 7,19 no Scene Graph; 1920×1080, 36,30 e 6,27 ms; 1080×1920, 36,60 e 5,99 ms. O RSS subiu de 25,9 para 60,8 MiB em 720p e de 30,3 para cerca de 74 MiB nos quadros de 8,3 MP. PSNR CPU/Scene Graph foi ~24,1 dB; a diferença visual ainda é perceptível. Os tempos CPU variaram entre repetições; o relatório guarda os nove resultados e as medianas.
- Conclusão: medir throughput não basta para aprovar migração. O raster atual permanece padrão até existir snapshot de cena compartilhado, maior paridade, testes adicionais e orçamento de memória definido.
- Melhoria concreta no compositor existente: cache limitado (128 entradas) de `QSvgRenderer`, invalidado por caminho/tamanho/data de modificação. Testes verificam reutilização parseada, quadro determinístico e invalidação após substituir o SVG.
- Verificação: `scripts/build.ps1 -Test` (4/4), `scripts/smoke.ps1` (preview/export 44,09 dB), `scripts/smoke-production.ps1` e `scripts/benchmark-renderers.ps1 -Frames 60 -Nodes 120` passaram.
- Referência arquitetural: [ADR 0032 — fundação do motor gráfico 2.0](architecture/0032-render-engine-2.md). O relatório e as imagens permanecem em `artifacts/renderer-benchmark-20261026-032146512/` e não são versionados.

### Entrega implementada: presets reutilizáveis para títulos e textos

- Um título ou texto estilizado pode ser salvo como preset de composição pela ação **Salvar seleção do editor** na biblioteca.
- O preset preserva texto, fonte, cor, tamanho, movimentos, duração e keyframes; depois pode ser instanciado na produção e continuar editável.
- O manifesto da biblioteca lista o ID real do preset para uso em receitas e o teste automatizado cobre salvar e reutilizar um título de impacto.

### Entrega implementada: cor de destaque independente da direção de arte

- `captionHighlightColor` configura separadamente a palavra falada na direção de arte global, por cena ou por shot; quando omitida em receitas antigas, acompanha `primaryColor`.
- O inspetor expõe o controle de cor, o render e a prévia usam o mesmo valor, e projetos novos persistem o campo no schema 14. Projetos schema 1–13 continuam carregando com comportamento anterior.
- A receita v3 aceita o campo dentro de `artDirection`; testes cobrem parsing, render schema 14 e compatibilidade de schemas existentes.

### Entrega implementada: fontes externas e presets visuais nomeados

- A biblioteca importa arquivos TTF/OTF válidos, guarda uma cópia própria e registra a família no aplicativo e no render-worker. Fontes excluídas podem ser restauradas e ficam disponíveis no seletor da direção de arte.
- O inspetor salva o estilo ativo como preset nomeado e aplica qualquer preset ao escopo global, cena ou shot. A biblioteca mostra uma amostra de tipografia e paleta; itens têm categorias, busca, restauração e exclusão reversível.
- Receitas v3 aceitam `artDirectionPreset` na raiz, cena e shot usando IDs do guia gerado na instalação. A herança continua global → cena → shot e campos inline substituem valores do preset. IDs de biblioteca são locais; a fonte precisa estar importada na máquina que renderiza.
- Testes automatizados cobrem cópia e aplicação de fontes disponíveis no Windows, salvamento/aplicação de estilos, herança nos três escopos e precedência de campos inline. Verificações do guia e smoke de workspace registram a interface real.
- Decisão arquitetural: [ADR 0029 — fontes externas e presets visuais](architecture/0029-library-fonts-and-visual-style-presets.md).

### Entrega implementada: ajuste de cortes e sobreposição na timeline

- A alça verde de um shot move o corte compartilhado entre dois planos e atualiza a duração dos dois shots vizinhos, elementos, beats e keyframes associados.
- A faixa âmbar da transição ganhou uma alça de redimensionamento: arraste para ajustar o overlap visual de 2 a 300 quadros, limitado à duração do shot de entrada. Corte e overlap formam uma única ação de desfazer/refazer por gesto.
- O projeto mantém cobertura contínua da cena; transições continuam sem acrescentar tempo à narração nem ao vídeo. A regra funciona nos projetos existentes e usa o campo de transição já persistido.
- Verificação: `scripts/build.ps1 -Test` passou (4/4), incluindo limites e undo/redo do gesto; `scripts/smoke-workspace.ps1` passou sem erros QML. A captura do editor com a receita v3 exibiu os três shots, a faixa dissolve e sua alça de overlap.

### Entrega implementada: ritmo de cortes guiado pela narração

- Receitas v3 aceitam `editing.rhythm: "narration"` e omitem `start` em todos os shots. O compilador usa timestamps reais das palavras sintetizadas, pontuação e pivôs textuais comuns para escolher cortes e equilibrar a duração dos planos conforme a intensidade.
- O cálculo respeita durações mínimas e transições. Elementos sem tempo explícito, incluindo filhos de grupos, começam junto ao corte escolhido; tempos explícitos de elementos, beats e keyframes continuam preservados e precisam caber no shot. O modo falha com uma mensagem clara se faltar alinhamento real ou espaço temporal.
- O ritmo é determinístico e baseado em sinais de texto; não substitui uma avaliação semântica livre por IA. Receitas manuais existentes e projetos antigos continuam preservados.
- Verificação: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke-production.ps1` passou com Microsoft Maria Desktop. A receita v3 de exemplo gerou 3 shots e 3 Visual Beats, abriu no editor e comparou preview/export em 44,27 dB. `scripts/update-ai-guide.ps1` regenerou o guia e `docs/capabilities-v1.json`.
- Decisão arquitetural: [ADR 0027 — ritmo de cortes guiado pela narração](architecture/0027-content-driven-shot-rhythm.md).

### Entrega implementada: cache de quadros e seek em vídeos

- O compositor mantém até 64 MiB de quadros de vídeo decodificados em cache LRU. A chave considera arquivo, revisão, tamanho do quadro, trim, velocidade, recorte, FPS e número do quadro; alterações nesses dados não reutilizam imagens antigas.
- Saltos grandes e buscas para trás reiniciam o decoder com seek temporal FFmpeg em vez de decodificar desde o início do clipe. O ponto de seek considera o primeiro quadro da fonte depois do trim, preservando o mesmo quadro que a sequência de exportação.
- A prévia de áudio usa o seek de `QMediaPlayer`: ao iniciar a reprodução, a posição da fonte é calculada pelo quadro, trim e velocidade da pista. Não há uma segunda implementação de decoder de áudio no compositor.
- Verificação: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke-production.ps1` passou com Microsoft Maria Desktop. No teste de seek fora de ordem, houve 2 hits de cache, 2 seeks diretos e apenas 3 quadros decodificados; o quadro da prévia comparado ao MP4 marcou 32,29 dB. O Director's Cut continuou com 2700 quadros e 42,46 dB entre prévia e exportação.
- Decisão arquitetural: [ADR 0025 — cache e seek de vídeo](architecture/0025-video-frame-cache-seek.md).

### Entrega implementada: presets reutilizáveis de câmera

- Receitas v3 podem declarar trajetórias nomeadas em `cameraPresets` e aplicá-las a um shot por `cameraPreset`. Cada preset define estados completos em segundos relativos ao início do shot; o compilador os converte em keyframes de câmera editáveis no projeto.
- A validação limita 32 nomes, 512 keyframes nas definições, 4096 keyframes expandidos por produção e exige que o último estado caiba no shot. O mesmo preset pode ser referenciado por vários shots.
- `cameraPreset` não pode ser combinado no mesmo shot com `focus.follow:true` nem com Visual Beats de câmera, evitando comandos concorrentes. Câmera sem preset, receitas v1/v2 e o schema de projeto permanecem compatíveis.
- Verificação: `scripts/build.ps1 -Test` passou (4/4), incluindo reutilização em dois shots e referências inválidas; `scripts/smoke-production.ps1` passou com Microsoft Maria Desktop e confirmou keyframes no exemplo v3 editável, com comparação prévia/exportação em 44,32 dB.
- O exemplo `examples/shots-visual-beats-v3.production.json` demonstra o campo. `docs/GUIA-PARA-IA.md` e `docs/capabilities-v1.json` foram regenerados.

### Entrega implementada: formatos paisagem e retrato

- A criação de projetos oferece paisagem 1280×720, paisagem Full HD 1920×1080 e retrato 1080×1920; o armazenamento preserva as dimensões sem alterar projetos anteriores.
- Receitas v3 aceitam `canvas` opcional na raiz com `width` e `height` inteiros. Apenas os três pares documentados são válidos; sem o campo, o padrão continua 1280×720. Coordenadas e dimensões dos elementos usam pixels do quadro escolhido, e a câmera sem coordenadas explícitas inicia no centro dele.
- Compilação de receitas, cenários nativos, legendas, preview compartilhado e render/export usam o mesmo quadro. Fundos nativos 16:9 mantêm a proporção e usam corte central para preencher um projeto vertical.
- Verificação: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke.ps1` e `scripts/smoke-workspace.ps1` passaram. `scripts/smoke-production.ps1` sintetizou a receita retrato com Microsoft Maria Desktop, renderizou o MP4 1080×1920 e abriu o projeto na interface; a comparação de still/export marcou 43,96 dB. Os testes cobrem parse, dimensões, câmera padrão, legenda, persistência e opções da tela de novo projeto.
- Exemplo: [retrato 1080×1920](../examples/retrato-1080x1920-v3.production.json). Decisão: [ADR 0024 — formatos de tela](architecture/0024-canvas-formats.md).

### Entrega implementada: áudio embutido em clipes de vídeo

- Ao selecionar uma camada de vídeo, **Adicionar áudio do vídeo à timeline** verifica a faixa de áudio local e cria uma pista vinculada ao início, duração, trim e velocidade do clipe. Alterações posteriores na camada mantêm a sincronização; mover/aparar a pista ou editar seu trim/velocidade a desvincula para edição independente. Arquivos sem faixa de áudio reconhecida são recusados com uma mensagem específica.
- A pista permanece editável: início, duração, ganho, mute, ducking, fades, automação, trim da fonte e velocidade. Prévia e exportação aplicam o mesmo trim/velocidade; a exportação decodifica a faixa de áudio 0 do arquivo de vídeo e a mistura com as demais fontes.
- Receitas v3 podem usar um arquivo de vídeo local como `audioTracks[].source` e declarar `sourceInSeconds` (0–3600, padrão 0) e `playbackRate` (0,25–4, padrão 1). Projetos com esses controles persistem no schema 13; schemas anteriores continuam abrindo.
- Verificação: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke.ps1`, `scripts/smoke-workspace.ps1` e `scripts/smoke-production.ps1` passaram. O smoke gerou MP4 com vídeo e seno de áudio, aplicou trim de 0,25 s e velocidade 1,5×, mediu áudio audível com 1 segundo na saída, comparou preview/export em 32,99 dB e confirmou a ação do inspetor, o vínculo e a persistência schema 13.
- Decisão arquitetural: [ADR 0023 — áudio embutido em vídeo](architecture/0023-embedded-video-audio.md).

### Entrega implementada: seleção múltipla na timeline

- Ctrl+clique adiciona ou remove clipes da seleção. O inspetor continua editando o item principal da seleção.
- Arrastar um clipe selecionado desloca o conjunto pelo mesmo delta, preserva os espaçamentos e respeita os limites da sequência e de cada cena. Grupos movem seus filhos e keyframes como parte do mesmo conjunto.
- Uma operação de movimento usa um único passo de desfazer/refazer. Os limites são calculados sobre a posição original do gesto para evitar deriva enquanto o ponteiro se move.
- Verificação: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke-workspace.ps1` passou, com a tela do editor aberta e sem erros QML. O teste do controlador cobre seleção aditiva, preservação de espaçamento, limite no início da sequência e undo/redo do conjunto. `scripts/update-ai-guide.ps1` regenerou o guia sem adicionar campos fictícios ao JSON.

### Entrega implementada: envelopes gráficos de áudio

- A timeline desenha a curva da automação de cada faixa sobre o intervalo temporal correspondente. Clique para adicionar um ponto; arraste pontos para alterar tempo e ganho com precisão por frame.
- Os pontos existentes continuam limitados a 128 por pista e aceitam os easings já validados pelo projeto. Um gesto inteiro é uma única ação de desfazer/refazer; a prévia aplica o valor editado enquanto o projeto permanece editável e serializado no schema 8.
- A receita IA mantém o mesmo contrato JSON. O guia explica que edição gráfica é uma capacidade manual do editor, não um campo novo.
- Verificação: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke-workspace.ps1` passou sem erros QML; `scripts/smoke-production.ps1` passou com a voz Microsoft Maria Desktop. Prévia/exportação marcou 40,41 dB no exemplo principal e 42,46 dB no Director's Cut; o projeto de 90 s abriu no editor. `scripts/update-ai-guide.ps1` regenerou o guia e o catálogo sem alterar o contrato da receita.

### Entrega implementada: correção manual dos tempos de palavra

- Legendas com alinhamento real agora exibem no inspetor início e fim de cada palavra em quadros absolutos do projeto, com entrada numérica e incremento unitário. Na faixa da legenda na timeline, os limites podem ser arrastados e ajustam-se ao quadro mais próximo.
- Ajustar uma borda compartilhada atualiza as duas palavras adjacentes; cada palavra conserva pelo menos um quadro e os limites permanecem dentro da legenda. O arraste inteiro é uma ação; edições inválidas são recusadas e a ação pode ser desfeita/refeita.
- Selecionar uma camada rola a timeline até a faixa correspondente. A lista de áudio rola separadamente, preservando espaço visível para shots e camadas mesmo em produções com várias pistas.
- Os intervalos continuam no campo `captionWords` e no schema 7; nenhuma propriedade nova foi acrescentada à receita. O guia da IA esclarece que a correção ocorre no projeto compilado.
- Verificação: `scripts/build.ps1 -Test` passou (4/4), `scripts/smoke-workspace.ps1` passou e `scripts/smoke-production.ps1` passou com Microsoft Maria Desktop. O smoke abriu uma legenda real de 13 palavras na cena ativa, confirmou a faixa selecionada e os controles QML, e renderizou o Director's Cut de 2700 quadros com preview/export em 42,46 dB. O teste do controlador verifica drag com frames sucessivos agrupados em um undo. `scripts/update-ai-guide.ps1` regenerou o guia e o catálogo.
- Snapping geométrico de clipes visuais/áudio, arraste de posição e trim do fim das pistas de áudio implementados nesta continuação.

### Entrega implementada: zoom temporal e navegação da timeline

- A timeline oferece zoom contínuo de 1× a 8×, aproximação centrada sob o ponteiro com Ctrl+roda e deslocamento horizontal com Shift+roda/barra.
- Régua, playhead, shots, beats, camadas e envelopes usam o mesmo intervalo visível. Scrubbing e arrastes convertem pixels para frames conforme o zoom atual.
- Verificação: `scripts/build.ps1 -Test` passou (4/4) e `scripts/smoke-workspace.ps1` passou sem erros QML. A captura confirma os controles de Zoom/Pan e os rótulos da régua. `scripts/update-ai-guide.ps1` gerou a documentação manual; a suíte ainda não simula gestos de Ctrl+roda nem movimento do slider.

### Entrega implementada: snapping geométrico de clipes visuais

- O botão **Snap ligado/desligado** controla o alinhamento durante o arraste. As bordas do conjunto de clipes visuais selecionados e as bordas de uma pista de áudio podem encaixar no playhead, limites de cena/shot, beats, outras camadas e outras pistas.
- A tolerância representa oito pixels na escala atual da timeline. Uma guia vertical âmbar mostra o frame alinhado; desligar o botão preserva o movimento livre.
- Seleções múltiplas e grupos mantêm o movimento sincronizado, limites válidos por cena e uma única ação undo/redo. A alça de áudio move a pista sem recriar seu player; uma segunda alça ajusta o fim/duração frame a frame. Beats de som vinculados acompanham o início. Nenhum campo novo foi adicionado à receita IA.
- Verificação: `scripts/build.ps1 -Test` passou (4/4), incluindo casos do controlador para alinhamento visual e de áudio dentro da tolerância, ausência de encaixe distante e movimento/undo sincronizado do beat sonoro; `scripts/smoke-workspace.ps1` passou sem erros QML; `scripts/smoke-production.ps1` passou com Microsoft Maria Desktop, exportou o exemplo principal e o Director's Cut de 2700 frames e abriu os projetos no editor. `scripts/update-ai-guide.ps1` regenerou o guia e o catálogo; o guia documenta os controles manuais sem atribuir campos JSON inexistentes.
- Decisão arquitetural: [ADR 0022 — snapping geométrico de clipes](architecture/0022-timeline-geometric-snapping.md).

### Entrega anterior: tipografia animada por palavra

- Receitas v3 aceitam `wordAnimation` com os presets `highlight`, `none`, `pop`, `rise`, `fade` e `pop-rise`. Escala inicial, deslocamento vertical e duração são configuráveis e usam timestamps reais da voz para cada palavra.
- O editor permite alterar o preset e os parâmetros em cada legenda alinhada. O projeto schema 12 persiste esses ajustes; o compositor compartilhado aplica os mesmos movimentos na prévia, still e exportação.
- Legendas sem timestamps reais permanecem estáticas e não aceitam os presets animados. O destaque de cor existente segue como padrão compatível.
- Verificação desta rodada: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke.ps1`, `scripts/smoke-workspace.ps1` e `scripts/smoke-production.ps1` passaram. O teste de produção usou Microsoft Maria Desktop, gerou o Director's Cut schema 12 com 2700 quadros, seis cenas, 18 shots e 20 beats; a comparação entre prévia e MP4 marcou 42,46 dB. O smoke conferiu `wordAnimation: pop-rise` em todas as legendas da receita e a abertura do projeto no editor. O teste unitário verificou persistência, animação visual em dois quadros e edição pelo inspetor.

### Entrega anterior: direção de arte por cena e shot

- Receitas v3 aceitam `artDirection` parcial na raiz, em cada cena e em cada shot. A resolução herda global → cena → shot; propriedades explícitas `color`, `secondaryColor` e `fontSize` dos elementos prevalecem.
- Os overrides são gravados nos marcadores do projeto schema 11. Preview, still e exportação aplicam o mesmo estilo ao fundo, tipografia, legendas e destaque de palavra; elementos e cenas preservam sua organização editável.
- O inspetor permite selecionar escopo global, cena atual ou shot atual. Ao criar um override no editor, o estilo efetivo do escopo pai é copiado como base; alterações futuras no pai não reescrevem um override já criado.
- Fontes externas e presets visuais nomeados ainda não são aceitos. No projeto gerado, cores e tamanho explícitos de um elemento continuam prioritários e não são recoloridos por uma edição posterior do estilo local.
- Verificação desta rodada: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke.ps1` e `scripts/smoke-production.ps1` passaram. O smoke usou Microsoft Maria Desktop, confirmou estilos da cena `financiamento` e do shot `financiamento-cet` no projeto schema 11 e comparou o Director's Cut de 2700 quadros entre preview e MP4 em 42,53 dB. O teste de armazenamento verificou que estilos de cena/shot persistem e que o override do shot altera a composição.

### Entrega anterior: trim, velocidade e recorte de vídeo

- Camadas de vídeo v3 aceitam trim de entrada `sourceInSeconds`, velocidade `speed` de 0,25× a 4×, saída `durationFrames` na timeline e `crop` normalizado da fonte.
- O compositor aplica a mesma sequência de filtros FFmpeg no preview, still e MP4. O inspetor ganhou controles para início da fonte, velocidade, origem e tamanho do recorte; o campo normal de duração continua ajustando a saída na timeline.
- Projetos com trim, velocidade ou crop personalizado usam schema 10; os projetos antigos de vídeo schema 5 continuam abrindo com início zero, velocidade normal e corte central.
- O guia e `capabilities-v1.json` descrevem esses campos, limites, defaults e o schema 10.
- Verificação: `scripts/build.ps1 -Test`, `scripts/smoke.ps1` e `scripts/smoke-production.ps1` passaram; o Director's Cut teve 2700 quadros e comparação preview/export em 42,61 dB. O clipe de vídeo com os três controles marcou 32,99 dB.

### Entrega anterior: efeitos sonoros disparados por Visual Beat

- A ação `sound` usa um arquivo local e ganho de 0–4 para criar uma pista de áudio editável sincronizada ao beat. Projetos schema 9 validam a associação e a timeline preserva o vínculo ao mover, cortar e excluir a pista.
- O Director's Cut de 90 segundos tem 20 beats e inclui `examples/media/soft-chime.wav`; o guia embute a receita e explica que o arquivo local precisa acompanhar o JSON.
- A compilação, 4/4 testes e smoke de produção passaram; a comparação preview/export marcou 42,61 dB.

### Sincronização visual por palavra concluída nesta rodada

- Legendas geradas a partir da narração agora guardam intervalos de palavra resolvidos pelos eventos reais do sintetizador pt-BR. A palavra ativa usa a cor primária da direção de arte e o mesmo compositor desenha prévia e exportação.
- Projetos schema 7 persistem esses intervalos; schemas 1–6 continuam aceitos e sem destaque sincronizado. Se duas palavras começarem no mesmo quadro de vídeo, o quadro destaca a última palavra daquele intervalo, devido à resolução temporal do vídeo.
- O teste automatizado cobre a resolução de oito palavras, salvamento/reabertura do projeto e mudança visual entre quadros de palavra.

### Camadas de vídeo implementadas nesta rodada

- O compositor compartilhado entre prévia e render agora decodifica quadros de vídeo local sob demanda usando o FFmpeg configurado pelo aplicativo. A imagem mantém proporção e preenche sua caixa com corte central.
- O editor oferece **Adicionar vídeo à composição** e reconhece MP4, MOV, MKV, WebM, AVI e M4V. A camada aparece como objeto selecionável e usa os controles de duração, posição e escala existentes.
- Projetos schema 5 persistem a camada sem alterar a leitura dos projetos anteriores. Receitas v3 aceitam `kind: "video"` com `source` local relativo ao JSON ou absoluto; links remotos não são aceitos.
- Guia IA, catálogo de capacidades e README descrevem formatos, comportamento e dependência de arquivo local/FFmpeg. Teste automatizado gera um clipe curto e verifica persistência e diferença entre quadros consecutivos.
- Limites atuais: a ação **Adicionar áudio do vídeo à timeline** extrai a faixa de áudio 0 e mantém o vínculo de sincronização com a camada até que a pista seja editada separadamente. Ainda não há waveform da fonte; em prévia, buscar para trás reinicia a decodificação do clipe.

### Concluído nesta rodada

- O modelo de projeto schema 4 grava e valida shots, Visual Beats e a associação de elementos a seus shots. Projetos anteriores continuam migrando e abrindo.
- Receitas `studio-dark-production` v3 importam várias composições dentro da mesma cena narrativa. O início de cada shot e beat usa tempos medidos da fala (`cueProgress`/`progress`) ou `offsetSeconds`; a duração do shot termina no próximo corte ou no fim da cena.
- Cortes secos trocam composição e câmera por shot. Beats `show`, `hide`, `transform` e `camera` são compilados em keyframes editáveis. A validação rejeita sequência com lacunas, sobreposições e beats fora do shot.
- A timeline exibe os shots e os marcadores dos Visual Beats, permite navegar até seus quadros e arrastar cortes. Um arraste atualiza durações, elementos, beats e câmera em uma só ação de desfazer.
- O inspetor de câmera grava keyframes vinculados ao shot ativo; keyframes acompanham o shot ao arrastar o corte.
- O exemplo `examples/shots-visual-beats-v3.production.json` foi sintetizado com voz pt-BR, compilado e renderizado em MP4; os quadros de shot/beat da exportação foram comparados com a prévia.
- `GUIA-PARA-IA.md` e `capabilities-v1.json` são gerados a partir das capacidades do aplicativo e identificam receita 3/projeto 4, limites e restrições reais.

### Implementado nesta continuação

- Câmera de um shot pode enquadrar automaticamente um elemento ou grupo usando `camera.focus.target` e uma margem `padding` configurável. Os limites do elemento são transformados através dos grupos ancestrais para calcular centro e zoom.
- Com `camera.focus.follow: true`, o compilador recalcula o enquadramento em todos os quadros do shot e grava os estados como keyframes editáveis. O modo não pode ser combinado com Visual Beats de câmera.
- Receitas v3 podem definir `editing.intensity` de 0 a 100. O valor determina movimento/timing de entrada e saída apenas para propriedades não informadas; instruções explícitas da IA e receitas sem esse campo preservam o comportamento anterior.
- Shots v3 aceitam `cut` ou transições `fade`, `dissolve`, `slide` e `zoom`, com duração de 2 a 300 quadros e direção para slide. O compositor sobrepõe as imagens na prévia e na exportação, a timeline marca a faixa entrante e o inspetor edita tipo/duração/direção do shot atual.
- Tipo, direção e duração de transição são salvos no projeto schema 4; projetos anteriores continuam assumindo `cut` sem duração.
- O foco é validado durante a importação: o alvo precisa existir dentro do mesmo shot; a margem aceita valores de 1 a 4. O foco define centro e zoom, preservando a rotação informada.
- O guia da IA, o manifesto de capacidades e a receita de demonstração descrevem auto reframe contínuo e intensidade 50.
- A compilação C++20 e os 4 testes automatizados passaram. O smoke de produção passou com voz Microsoft Maria Desktop: 6 cenas, 1517 quadros, exemplo v3 com 3 shots/3 beats, acompanhamento contínuo de câmera, intensidade 50 e dissolve entre shots; comparação preview/export em 43,82 dB.

### Direção de arte global, por cena e por shot

- Receitas v3 aceitam `artDirection` global e overrides parciais em cena e shot. A herança global → cena → shot mantém somente os valores realmente sobrescritos pelo escopo local.
- O compilador preserva cor, cor secundária e tamanho de fonte explicitamente declarados nos elementos; o estilo resolvido controla fundo, objetos com defaults, cabeçalhos, legendas e destaque de palavra.
- Projetos com estilos locais usam schema 11. A prévia e o render resolvem o mesmo estilo no shot ativo; no inspetor, a pessoa escolhe global, cena atual ou shot atual e pode desfazer a mudança.
- Alterações posteriores no escopo pai não reescrevem um override já salvo. No editor, a primeira edição local copia o estilo efetivo do pai como base. Cores e tamanhos explicitamente escritos em objetos não são recalculados por edições posteriores.
- Fontes externas, presets visuais nomeados e prévia de paletas estão disponíveis na biblioteca e no inspetor. A família de fonte é carregada localmente para a prévia e a exportação.

### Sincronização por palavra implementada parcialmente

- A síntese Windows pt-BR recolhe eventos `SpeakProgress` com palavra, posição no texto e tempo do fluxo de áudio; o worker registra esses dados junto aos arquivos WAV.
- Receita v3 aceita `wordIndex` no início de shot, no início de elemento, em keyframes e no tempo de Visual Beat, em base zero e ligado ao `cue` da mesma cena. O compilador resolve o evento em quadros e grava os estados no projeto editável.
- Índices sem alinhamento real são rejeitados. Quando o plano de duração fixa acelera uma fala, as posições acompanham o mesmo fator temporal.
- Os eventos SAPI podem ficar ligeiramente fora do tamanho final do WAV devido à diferença entre o relógio reportado e o arquivo; o worker calibra a sequência para o intervalo audível real, preservando a ordem e deixando espaço para o fim da última palavra.
- Os testes cobrem receita, timestamp e resolução em shot, beat, elemento e keyframe. O exemplo v3 usa `wordIndex`; o smoke de produção passou com voz Microsoft Maria Desktop e renderizou os três shots e três beats, incluindo os eventos acionados por palavra. Prévia/exportação do exemplo v3: 43,82 dB.
- Legendas de narração têm destaque da palavra falada pela cor primária; seus tempos ficam gravados no schema 7. A camada de legenda usa layout por métricas de fonte e respeita quebra de linha.
- O inspetor permite corrigir início/fim de cada palavra alinhada em quadros depois da compilação. Mover um limite compartilhado ajusta as duas palavras vizinhas; intervalos inválidos são recusados e cada ajuste entra no histórico undo/redo. Os timestamps corrigidos continuam persistidos no projeto schema 7.
- A correção visual do alinhamento em quadros já está disponível no inspetor e nas alças da faixa de legenda na timeline. Ainda falta escolher uma cor de destaque independente da direção de arte. A importação de uma produção precisa sintetizar a mesma fala com a voz local para obter tempos confiáveis.

### Controle de intensidade no editor

- O inspetor aplica um perfil de intensidade 0–100 aos elementos visuais da cena atual, calculando tipo/duração de entrada e saída com a mesma escala adotada pela receita v3.
- O comando entra no histórico como uma edição única e pode ser desfeito/refeito. Intensidade zero desliga movimentos automáticos; camadas de vídeo e grupos não são modificados.
- Na receita, a intensidade continua sendo a forma de configurar o perfil automaticamente antes da compilação. No editor, a aplicação é explícita para não substituir movimentos sem ação do usuário.

### Áudio em múltiplas pistas implementado parcialmente

- Projetos schema 4–10 salvam até 32 faixas independentes com caminho local, início/duração em frames, ganho, mute, ducking e automação configurável. Fades exigem schema 6 ou superior; automação de ganho exige schema 8. Os arquivos antigos continuam carregando sem migração destrutiva.
- A timeline importa, remove, silencia, reposiciona, altera duração/ganho, liga ducking e permite definir fades lineares de entrada/saída. Na prévia, cada pista toca sincronizada com o playhead e segue os fades.
- A prévia e a exportação calculam a mesma envoltória RMS suavizada por quadro quando a referência é WAV PCM mono a 24 kHz; a exportação aplica o envelope determinístico na pista. Referências em outros formatos usam sidechaincompress do FFmpeg como fallback.
- Receitas v3 podem declarar pistas por cena com fonte local, sincronizar início por cue/progresso/wordIndex e definir `fadeInFrames`/`fadeOutFrames` até 1800 quadros cada, limitados ao intervalo da pista. Projetos com fades usam schema 6; os fades também são aplicados no render por FFmpeg.
- O smoke compara mix sem/com ducking: RMS durante a voz caiu de 0,0414 para 0,0380, e após a fala os dois ficaram em 0,0324 e 0,0321. Na última verificação estéreo, os fades mediram 0,0045 no começo e 0,0050 no fim contra 0,0229 no trecho estável; automação por pontos/easing mediu 0,0025 no início e 0,0226 após a rampa.

### Situação por fase

| Fase | Situação | O que falta para fechar |
|---|---|---|
| 1 — Fundação | Concluída | Teste temporal cobre 20 cenas, 120 shots, 240 beats concorrentes e cortes de borda; smoke de produção valida receita v3 e projeto editável. |
| 2 — Edição dinâmica | Concluída | Auto reframe contínuo, intensidade na receita/editor, transições fade/dissolve/slide/zoom, presets de câmera, ritmo de cortes guiado pela narração e edição de duração/overlap na timeline estão implementados. |
| 3 — Acabamento audiovisual | Concluída | Direção de arte por escopo, fontes importadas, presets visuais nomeados, tipografia por palavra, áudio em múltiplas pistas e efeitos sonoros estão implementados. Presets de títulos/textos usam a biblioteca pessoal de composições e permanecem editáveis depois da importação. |
| 4 — Recursos avançados | Concluída | Todos os 12 itens desta fase estão implementados e verificados. Regiões de documentos/PDF, conectores ancorados, trajetórias Bézier e mais ferramentas de timeline ficam como propostas futuras fora do checklist atual. |

### Limites confirmados nesta versão

- `transitionIn` aceita a string legada `cut` ou um objeto com `type`, `durationFrames` e, para slide, `direction`. O inspetor permite ajustar tipo, duração e direção no shot atual. A sobreposição é visual e não prolonga a duração do áudio/cena; o shot de saída congela no último quadro durante a transição. Fade atravessa preto intencionalmente.
- Beats aceitam `show`, `hide`, `transform`, `camera` e `sound`. Um beat `sound` cria uma pista local de uma execução, vinculada ao quadro sincronizado; a fonte não é copiada para o JSON e precisa continuar acessível no computador.
- Shot, Visual Beat, início de elemento, keyframe e destaque de legenda podem usar `wordIndex`, resolvido pelos eventos SAPI da voz local e guardado como quadros/intervalos no projeto. Legendas alinhadas podem ter cada limite de palavra corrigido numericamente no inspetor ou arrastado na faixa correspondente da timeline, com snapping por quadro.
- A timeline mostra e permite navegar pelos shots/beats; as alças movem o corte com ajuste dos conteúdos e um único undo. O inspetor grava câmera no shot ativo, edita a transição de entrada e preserva os keyframes ao mover o corte.
- Com Snap ligado, arrastar clipes visuais alinha início/fim ao playhead, limites de cenas/shots, beats, outras camadas e inícios/fins de áudio. A tolerância equivale a oito pixels e acompanha o zoom; uma guia âmbar mostra o encaixe. O botão desliga o snap para movimento livre. A ferramenta não acrescenta campos à receita IA; o movimento continua undoável como um gesto.
- A alça ↔ no início do envelope move a pista; a alça direita altera o fim/duração por frame. Ambas usam as mesmas referências de snap e cada gesto tem um único undo. Efeitos de som ligados a Visual Beats acompanham a mudança do início.
- Vídeo local como camada já existe, com quadros sincronizados à sequência. Áudio próprio do vídeo não é importado automaticamente e mídias remotas não são baixadas. Áudio em múltiplas pistas/ducking, direção de arte por escopo e controles de intensidade já existem. Regiões de documentos/PDF e conectores ancorados são evoluções futuras.
- Auto reframe por ID pode acompanhar o alvo em todos os quadros do shot usando `follow:true`. Esse modo gera keyframes de câmera e é incompatível com Visual Beats de câmera no mesmo shot. Presets de trajetória reutilizáveis já são aceitos em `cameraPresets`; trajetórias Bézier personalizadas continuam pendentes.
- `editing.intensity` orienta entradas/saídas automáticas quando a receita omite esses campos e ajusta o equilíbrio temporal/semântico do modo `editing.rhythm:"narration"`. Esse modo escolhe entre timestamps reais de palavras, favorecendo pontuação e pivôs textuais; não faz interpretação semântica livre. No editor, o comando de intensidade aplica o perfil explicitamente à cena atual.

### Próxima entrega recomendada

As quatro fases do checklist original estão concluídas. O plano global segue aberto até concluir estas evoluções:

- [x] Cor de destaque de palavra independente da direção de arte.
- [x] Presets reutilizáveis para títulos e textos, usando composições editáveis da biblioteca pessoal.
- [ ] Trajetórias de câmera Bézier personalizadas.
- [ ] Inserção de documentos/PDF com seleção de regiões.
- [ ] Conectores ancorados a objetos.
- [ ] Ferramentas avançadas de timeline; definir critérios de aceite antes de considerar esta área concluída.

---

## 1. OBJETIVO PRINCIPAL

Quero transformar o Studio Dark em uma ferramenta avançada de criação e edição automática de vídeos dirigidos por inteligência artificial.

Atualmente, o aplicativo permite importar produções JSON, gerar narração local, criar cenas com elementos nativos, aplicar movimentos, keyframes, grupos, câmera e exportar vídeos.

Essa estrutura deve ser preservada e evoluída.

**Meu objetivo é permitir que uma IA externa, como o ChatGPT, funcione como diretora, roteirista, animadora e editora audiovisual.**

A IA deve conseguir produzir um arquivo JSON com instruções detalhadas sobre:

- O que deve aparecer na tela.
- Quando cada elemento deve aparecer e desaparecer.
- Quando a câmera deve se movimentar.
- Quando deve ocorrer um corte.
- Como devem funcionar as transições.
- Como as palavras devem ser destacadas.
- Como os objetos devem ser animados.
- Quando efeitos sonoros devem acontecer.
- Como os elementos devem acompanhar a narração.
- Qual deve ser a duração de cada plano.
- Qual deve ser a duração total do vídeo.
- Qual estilo visual deve ser utilizado.
- Qual ritmo de edição deve ser aplicado.

O Studio Dark será responsável por interpretar essas instruções, produzir a narração, sincronizar todos os elementos e montar o vídeo automaticamente.

**Não quero apenas mais animações. Quero um verdadeiro motor de montagem audiovisual programável por JSON.**

---

## 2. REGRAS IMPORTANTES DE DESENVOLVIMENTO

Antes de implementar qualquer modificação:

1. Analise a arquitetura atual do Studio Dark.
2. Identifique os módulos responsáveis por importação JSON, síntese de voz, sincronização, renderização, câmera, grupos, animações, timeline e exportação.
3. Identifique o que já existe, o que pode ser reutilizado e o que precisa ser desenvolvido.
4. Preserve os recursos existentes, incluindo projetos e receitas compatíveis.
5. Evite reescrever o aplicativo inteiro sem necessidade.
6. Implemente progressivamente, mantendo o aplicativo funcional.
7. Crie testes para os recursos novos e para evitar regressões.
8. Não apresente funcionalidades como concluídas se ainda não funcionarem.

As novas funções devem ser integradas ao editor existente e, sempre que tecnicamente possível, permanecer editáveis pelo usuário.

Não quero apenas exemplos fictícios de implementação, pseudocódigo ou documentação. Quero as funcionalidades realmente implementadas no projeto.

Se alguma função depender de bibliotecas adicionais, avalie a compatibilidade com Windows e com a tecnologia já utilizada pelo aplicativo.

---

# PARTE 1 — NOVA ARQUITETURA DE MONTAGEM

## 3. CRIAR UM SISTEMA DE SHOTS (PLANOS VISUAIS)

Esta é uma das mudanças mais importantes.

Atualmente, o JSON organiza a produção principalmente em cenas contendo narração, elementos e câmera.

Quero adicionar o conceito de **plano visual (shot)**.

Uma cena narrativa poderá conter vários planos visuais independentes.

Exemplo:

Uma cena de 12 segundos apresenta um carro à venda.

Durante esses 12 segundos, quero que o aplicativo consiga executar:

- Plano 1: carro ocupando a tela inteira.
- Plano 2: corte para o preço anunciado.
- Plano 3: aproximação da expressão do personagem.
- Plano 4: corte para documentos e despesas.

Tudo isso deve acontecer dentro de uma única cena narrativa, sem precisar interromper ou duplicar a narração.

Cada shot poderá ter:

- Identificador único.
- Duração planejada ou relativa.
- Início e término.
- Composição visual própria.
- Elementos e grupos.
- Animações.
- Câmera independente.
- Transição de entrada e saída.
- Relação com a narração.
- Efeitos sonoros associados.
- Instruções visuais de direção.

Os planos devem permitir cortes secos, mudanças de enquadramento e transições.

A IA deve conseguir planejar a montagem com precisão.

**Importante:** a implementação não deve tratar cada shot como uma nova cena obrigatória. Uma cena pode conter diversos shots sem interromper a fala.

---

## 4. CRIAR VISUAL BEATS

Quero implementar um sistema chamado **Visual Beats**.

Um Visual Beat representa uma pequena ação visual sincronizada com um momento da narração.

Exemplo:

Narração:

"Lucas encontrou um carro barato. Mas havia um problema escondido."

Visual Beats:

1. O carro aparece.
2. O preço entra em destaque.
3. A câmera se aproxima do carro.
4. O preço desaparece.
5. Um aviso aparece na tela.
6. A câmera muda para o personagem preocupado.

Esses eventos devem ser organizados dentro dos shots.

Cada beat poderá controlar:

- Aparição de objetos.
- Desaparecimento de objetos.
- Movimentação.
- Mudança de escala.
- Opacidade.
- Rotação.
- Destaque de texto.
- Mudança de câmera.
- Corte visual.
- Transição.
- Disparo de efeito sonoro.

Quero poder controlar esses eventos por JSON.

Os Visual Beats devem permitir construir vídeos com novidades visuais frequentes, sem precisar criar dezenas de cenas narrativas.

---

# PARTE 2 — SINCRONIZAÇÃO PRECISA COM A NARRAÇÃO

## 5. CRIAR UM MOTOR INTELIGENTE DE SINCRONIZAÇÃO

Atualmente, o Studio Dark utiliza cues e mede a duração real da voz gerada.

Quero evoluir essa arquitetura para permitir que a IA planeje a duração dos vídeos e que o aplicativo tente cumprir esse planejamento.

Implementar dois modos:

### Modo A — Duração natural

O aplicativo:

1. Recebe o roteiro.
2. Gera a narração.
3. Mede a duração real das falas.
4. Organiza os shots e Visual Beats.
5. Sincroniza a montagem com a voz gerada.

### Modo B — Duração-alvo

A IA informa uma duração desejada, por exemplo 90 segundos.

O aplicativo deverá:

1. Receber o roteiro e o planejamento de tempo.
2. Gerar os áudios.
3. Medir a duração real.
4. Comparar a duração gerada com a desejada.
5. Distribuir o tempo entre falas, pausas, cenas e shots.
6. Ajustar a velocidade da voz dentro de limites configuráveis.
7. Preservar o tom da voz, evitando alterações artificiais de pitch.
8. Adaptar as animações aos tempos resolvidos.
9. Sincronizar legendas e efeitos.
10. Informar quando a duração solicitada for inviável sem prejudicar a naturalidade.

A duração-alvo não deve forçar uma narração absurdamente rápida.

Quero que o aplicativo utilize um relógio de referência único para áudio, vídeo e animações, com conversão consistente para frames de exportação.

A duração final deve ser calculada a partir dos dados reais da síntese, não de timestamps imaginados pela IA.

---

## 6. CRIAR SINCRONIZAÇÃO PROPORCIONAL

Além do `cue` e do `offsetSeconds` existentes, quero um sistema de eventos por porcentagem da fala.

Exemplo conceitual:

- 0%: o carro aparece.
- 25%: a câmera começa a aproximar.
- 50%: aparece o preço.
- 75%: o personagem reage.
- 100%: o plano termina.

Se a fala durar 4 segundos ou 6 segundos, os eventos deverão se adaptar proporcionalmente.

O nome definitivo dos campos JSON deverá ser escolhido durante a implementação e documentado claramente.

Os recursos atuais de `cue`, `offsetSeconds`, `keyframes` e `easing` precisam continuar funcionando.

---

## 7. SINCRONIZAÇÃO POR PALAVRA

Quero também estudar e, se viável, implementar alinhamento entre texto e áudio.

Exemplo:

Narração:

"Mas o verdadeiro problema estava no preço."

Quando a voz pronunciar "problema", o aplicativo poderá:

- Aplicar um zoom.
- Destacar a palavra na legenda.
- Exibir uma informação.
- Executar um efeito sonoro.

Isso exige identificar o momento real das palavras no áudio.

Não quero que o aplicativo adivinhe esses tempos contando caracteres.

Avalie uma solução de alinhamento de fala que funcione em português brasileiro, preferencialmente localmente e compatível com Windows.

Se não for possível obter um alinhamento confiável, mantenha a sincronização por fala e não simule uma precisão inexistente.

---

# PARTE 3 — MOTOR DE EDIÇÃO DINÂMICA

## 8. IMPLEMENTAR DYNAMIC EDIT ENGINE

Quero um sistema capaz de interpretar as instruções de edição fornecidas pela IA.

Esse motor deve controlar:

- Cortes.
- Duração dos planos.
- Enquadramentos.
- Movimentação de câmera.
- Revelações de elementos.
- Transições.
- Efeitos visuais.
- Tipografia.
- Ritmo geral do vídeo.

Quero que seja possível criar vídeos com dinâmica semelhante à de conteúdos modernos de YouTube, vídeos explicativos e documentários.

Não quero que o vídeo fique visualmente parado por longos períodos sem intenção narrativa.

Entretanto, não quero um sistema que aplique cortes aleatórios a cada dois segundos.

**A edição deve obedecer ao significado da narração e ao planejamento enviado pela IA.**

Como referência inicial:

- Mudanças visuais leves a cada 2–4 segundos.
- Mudanças de composição em intervalos de 5–8 segundos.
- Mudanças narrativas em intervalos maiores, conforme a história.

Esses valores devem ser configuráveis, não obrigatórios.

Momentos dramáticos podem ter planos longos.

---

## 9. CONTROLE DE INTENSIDADE DE EDIÇÃO

Quero que a IA possa definir a intensidade da edição.

Criar um parâmetro configurável de 0 a 100 ou uma configuração equivalente.

Exemplos:

**0–30: Cinematográfico**

- Poucos cortes.
- Movimentos suaves.
- Transições discretas.
- Composições mais contemplativas.

**31–70: Dinâmico**

- Cortes frequentes, mas justificados.
- Destaques visuais.
- Zooms moderados.
- Movimentação de objetos.
- Revelações progressivas.

**71–100: Acelerado**

- Cortes mais rápidos.
- Mudanças frequentes de enquadramento.
- Tipografia expressiva.
- Efeitos visuais pontuais.
- Maior energia audiovisual.

O usuário também deverá conseguir alterar esse valor pelo editor, quando possível.

A intensidade deve orientar o comportamento automático, mas não substituir instruções explícitas da IA.

---

## 10. AUTO REFRAME E CÂMERA AVANÇADA

Quero evoluir o sistema de câmera existente.

Além dos keyframes atuais, permitir:

- Enquadramentos independentes por shot.
- Cortes instantâneos de câmera.
- Zoom-in e zoom-out.
- Movimentos de pan.
- Movimentos suaves entre posições.
- Foco em um elemento ou grupo.
- Diferentes curvas de interpolação.
- Presets de movimentação reutilizáveis.

Exemplo:

A IA pode solicitar:

"Aproxime a câmera do preço enquanto a narração menciona o valor."

O JSON deve representar essa intenção com parâmetros objetivos.

O motor deve calcular a transformação de câmera correspondente e executar o movimento corretamente.

Manter a compatibilidade com as curvas de easing já existentes.

A implementação precisa lidar com coordenadas locais e globais de grupos aninhados, evitando focos incorretos.

---

## 11. TRANSIÇÕES INTELIGENTES

Quero um sistema de transições entre shots.

Implementar inicialmente:

- Corte seco.
- Fade.
- Dissolve.
- Slide horizontal.
- Slide vertical.
- Zoom transition.

As transições devem ter duração configurável.

Preciso poder definir a transição pelo JSON e visualizar seu resultado no editor.

O aplicativo deve tratar corretamente a sobreposição temporal de planos durante transições e não produzir intervalos pretos indesejados.

Evite transições excessivamente chamativas como padrão. Cortes secos devem continuar sendo uma opção importante.

---

# PARTE 4 — EVOLUÇÃO VISUAL

## 12. SUPORTE A VÍDEOS COMO CAMADAS

O guia atual documenta imagens e SVG, mas não vídeo como camada.

Quero adicionar suporte a arquivos de vídeo locais.

A IA poderá combinar:

- Vídeos reais.
- Objetos vetoriais.
- Imagens.
- Personagens.
- Formas.
- Texto.
- Grupos.

Os vídeos devem aceitar, conforme viabilidade técnica:

- Posicionamento.
- Escala.
- Corte e ajuste ao enquadramento.
- Rotação.
- Opacidade.
- Início e término de reprodução.
- Trim.
- Loop.
- Controle de áudio.
- Sobreposição com outros elementos.
- Integração com a timeline.

Quero utilizar vídeos reais como B-roll, misturando-os com animações do Studio Dark.

Priorize suporte a arquivos locais e evite depender de download automático de URLs.

O app deve informar quando um arquivo estiver ausente, incompatível ou corrompido.

---

## 13. BIBLIOTECA VISUAL E PRESETS

Quero preservar e expandir a biblioteca pessoal existente.

O aplicativo já possui suporte a elementos pessoais e presets compostos.

Quero permitir presets reutilizáveis para:

- Aberturas.
- Textos animados.
- Personagens.
- Cartões informativos.
- Comparações.
- Gráficos ilustrativos.
- Transições.
- Composições completas.
- Elementos de destaque.

O guia exportado deverá incluir os identificadores realmente disponíveis na biblioteca pessoal.

A IA não deverá precisar inventar IDs de elementos.

Também quero manter a possibilidade de editar os componentes internos de uma composição importada.

---

## 14. SISTEMA DE TIPOGRAFIA ANIMADA

Quero legendas e textos mais sofisticados.

Funcionalidades desejadas:

- Animação de entrada e saída.
- Destaque de palavras.
- Mudança de cor.
- Mudança de escala.
- Movimento vertical e horizontal.
- Palavras-chave em destaque.
- Presets de títulos.
- Textos grandes de impacto.
- Animação de números e informações.
- Integração com o alinhamento da narração.

**Já implementado:** quando há timestamps reais da voz pt-BR, o compositor destaca a palavra correspondente por quadro e salva o alinhamento no projeto schema 7. Presets por palavra incluem `highlight`, `none`, `pop`, `rise`, `fade` e `pop-rise`; duração, escala inicial e deslocamento são editáveis no inspetor. O alinhamento de início/fim pode ser corrigido em quadros no inspetor ou arrastado na faixa da timeline. Sem alinhamento real, mantém a legenda estática. Permanecem pendentes presets reutilizáveis para títulos/textos e uma cor de destaque independente da direção de arte.

Garantir que o texto permaneça legível e não invada regiões importantes da composição.

---

## 15. DIREÇÃO DE ARTE GLOBAL

Quero que a IA possa definir um estilo visual consistente para a produção.

Exemplo de parâmetros:

- Paleta de cores.
- Tipografia.
- Tamanho dos títulos.
- Estilo de legendas.
- Estilo de cartões.
- Estilo de animação.
- Intensidade de movimento.
- Margens de segurança.
- Posição padrão das legendas.
- Tratamento visual dos elementos.

Essas propriedades devem funcionar como padrões globais, podendo ser sobrescritas em cenas ou shots.

A ideia é evitar que cada cena pareça pertencer a um vídeo completamente diferente.

---

# PARTE 5 — ÁUDIO PROFISSIONAL

## 16. MOTOR DE DESIGN SONORO

Quero evoluir o sistema de áudio existente.

Atualmente, o guia documenta narração local, música procedural e sinais de troca de cena.

Quero permitir:

- Trilhas de música importadas.
- Efeitos sonoros locais.
- Efeitos associados aos Visual Beats.
- Controle de volume.
- Fade-in e fade-out.
- Posicionamento temporal.
- Sincronização com cortes e animações.
- Mixagem de múltiplas fontes.
- Redução automática da música durante a narração (ducking).

Exemplo:

Quando uma informação importante aparece, o aplicativo toca um efeito discreto.

Quando a narração começa, a trilha diminui de volume.

Quando a narração termina, o volume pode voltar ao normal.

Quero que a IA possa especificar esses comportamentos no JSON.

Se um efeito não existir na biblioteca, o aplicativo não deve inventar um arquivo.

Os caminhos e os IDs precisam ser verificados.

---

# PARTE 6 — NOVO FORMATO DE PRODUÇÃO JSON

## 17. EVOLUIR O FORMATO DE FORMA ORGANIZADA

Quero que você proponha e implemente uma evolução do formato de produção.

O formato atual utiliza:

- `format: studio-dark-production`
- `version: 2`
- `catalogVersion: 1`

O guia também documenta projeto schema 3.

Avalie criar uma nova versão de produção para os recursos avançados, mantendo suporte às receitas anteriores.

A estrutura deverá conseguir representar conceitualmente:

**Produção**
- Configurações globais.
- Estilo visual.
- Configuração de narração.
- Duração-alvo.
- Intensidade de edição.
- Configurações de áudio.
- Fontes e mídia.

**Cenas**
- Narração.
- Duração planejada.
- Contexto visual.
- Lista de shots.

**Shots**
- Composição.
- Duração.
- Câmera.
- Elementos.
- Transições.
- Animações.

**Visual Beats**
- Evento.
- Referência temporal.
- Elemento afetado.
- Propriedades alteradas.
- Efeito sonoro, quando aplicável.

**Sincronização**
- Tempo por fala.
- Progresso relativo.
- Eventos por palavra, quando suportados.
- Duração final resolvida.

O esquema definitivo deve evitar propriedades redundantes e ambiguidades entre scene, shot, cue e beat.

Defina regras claras de herança, duração, ordem de renderização e resolução de conflitos.

**Não use esses nomes como se já existissem no aplicativo. Eles são requisitos conceituais para o novo design.**

---

# PARTE 7 — EDITOR E VALIDAÇÃO

## 18. TIMELINE EDITÁVEL

Após a importação do JSON, quero que o usuário possa modificar a produção.

Sempre que viável, o editor deverá permitir:

- Ajustar a duração de shots.
- Mover cortes.
- Alterar transições.
- Editar movimentos de câmera.
- Ajustar elementos.
- Alterar texto.
- Modificar volume.
- Ajustar efeitos sonoros.
- Visualizar narração e eventos temporais.
- Editar presets sem destruir sua hierarquia.

Não quero que a produção seja transformada em um vídeo fechado imediatamente.

Ela deve continuar sendo um projeto editável do Studio Dark.

---

## 19. PRÉVIA E VALIDAÇÃO AUTOMÁTICA

Quero um sistema de validação mais robusto.

Antes de montar ou exportar, o aplicativo deverá detectar:

- JSON inválido.
- Campos desconhecidos.
- IDs duplicados.
- Recursos indisponíveis.
- Arquivos ausentes.
- Tempos impossíveis.
- Keyframes fora da duração.
- Eventos fora de shots.
- Animações incompatíveis.
- Transições inválidas.
- Referências a palavras sem alinhamento.
- Incompatibilidades de versões.
- Elementos fora do enquadramento.
- Textos potencialmente ilegíveis.
- Sobreposição com a faixa de legendas.

Os erros devem explicar qual cena, shot, beat ou elemento causou o problema.

Quando possível, ofereça uma prévia da montagem antes da exportação.

A prévia e o vídeo exportado devem utilizar a mesma lógica de sincronização.

---

## 20. EVOLUIR OS LIMITES TÉCNICOS COM SEGURANÇA

O guia atual documenta:

- 1280 × 720.
- 30 FPS.
- Até 20 cenas.
- Até 200 elementos.
- Até 10 trechos de narração por cena.
- Até 150 caracteres por trecho.
- Até 8 níveis de grupos aninhados.

Quero que você avalie se esses limites continuam adequados após a criação de shots e Visual Beats.

Se necessário, implemente uma arquitetura que permita muitos eventos temporais sem criar centenas de elementos simultâneos.

Os limites só devem ser aumentados quando houver suporte real e testes de desempenho.

**Implementado nesta rodada:** receitas v3 e novos projetos aceitam 1280×720, 1920×1080 e 1080×1920; 1280×720 continua sendo o padrão e projetos horizontais existentes mantêm suas dimensões. Os limites restantes de cenas, nós, falas, texto e profundidade de grupos continuam iguais e são validados pelo importador. A próxima avaliação desses limites deve usar medições de produções longas, sem aumentá-los por antecipação.

---

# PARTE 8 — FLUXO FINAL DE PRODUÇÃO

## 21. COMO QUERO QUE O STUDIO DARK FUNCIONE

Este deve ser o fluxo ideal:

**Etapa 1 — Direção pela IA**

O usuário envia um roteiro e o guia atualizado ao ChatGPT.

O ChatGPT desenvolve a direção audiovisual e gera um arquivo JSON com:

- Roteiro preservado.
- Cenas.
- Shots.
- Visual Beats.
- Movimentos de câmera.
- Animações.
- Transições.
- Efeitos.
- Direção de arte.
- Planejamento temporal.

**Etapa 2 — Importação**

O Studio Dark lê o JSON, valida o conteúdo e identifica todos os recursos.

**Etapa 3 — Narração**

O aplicativo sintetiza a voz, mede os áudios e obtém os tempos reais.

**Etapa 4 — Resolução temporal**

O motor combina:

- Duração da voz.
- Duração-alvo.
- Duração dos shots.
- Pausas.
- Eventos temporais.
- Animações.
- Transições.

**Etapa 5 — Montagem**

O Studio Dark constrói o vídeo com todos os elementos sincronizados.

**Etapa 6 — Prévia e edição**

O usuário visualiza o resultado, modifica elementos e ajusta a timeline.

**Etapa 7 — Exportação**

O aplicativo renderiza o vídeo final preservando a sincronização.

Quero que esse fluxo seja o principal objetivo técnico de toda a evolução.

---

# PARTE 9 — ORDEM DE IMPLEMENTAÇÃO

Não tente implementar todos os recursos de uma só vez sem garantir estabilidade.

Trabalhe por etapas.

## FASE 1 — FUNDAÇÃO DO NOVO MOTOR

Prioridade máxima:

1. [x] Analisar e documentar a arquitetura atual.
2. [x] Criar cenas, shots independentes e Visual Beats.
3. [x] Resolver um modelo temporal por quadros, com validação de intervalos.
4. [x] Sincronizar shots com a fala sintetizada e com `wordIndex` real.
5. [x] Implementar cortes entre shots.
6. [x] Integrar shots/beats à timeline, inspetor e histórico de desfazer.
7. [x] Preservar a leitura de receitas e projetos anteriores.
8. [x] Ampliar testes para produções longas com muitos shots, cortes no limite e eventos concorrentes.

Resultado esperado: conseguir criar várias composições e cortes dentro de uma mesma cena narrativa.

## FASE 2 — EDIÇÃO DINÂMICA

Implementar:

1. [x] Motor de edição dinâmica com intensidade configurável na receita e no editor.
2. [x] Auto Reframe por alvo, inclusive acompanhamento quadro a quadro.
3. [x] Transições fade, dissolve, slide e zoom editáveis.
4. [x] Sincronização temporal de animações com fala/progresso/palavra.
5. [x] Cortes e ritmo escolhidos automaticamente com base na narração, usando timestamps reais de palavras, pontuação, pivôs textuais e intensidade para equilibrar as durações dos shots.
6. [x] Presets reutilizáveis de trajetórias de câmera definidos na receita v3 e aplicados a um ou mais shots.
7. [x] Edição avançada de duração/overlap da sequência no editor, com movimento de corte, resize gráfico de overlap e undo/redo por gesto.

Resultado esperado: criar vídeos com ritmo de edição profissional usando os elementos já existentes.

## FASE 3 — ACABAMENTO AUDIOVISUAL

Implementar:

1. [x] Tipografia animada com presets de movimento/escala e controle por palavra, editável no inspetor e sincronizada aos timestamps reais.
2. [x] Destaque de palavra sincronizado quando existe alinhamento real de voz.
3. [x] Efeitos sonoros disparados por Visual Beat.
4. [x] Trilhas importadas em múltiplas pistas, com fades e ducking.
5. [x] Direção de arte global com paleta, fonte instalada e parâmetros de legenda.
6. [x] Direção de arte por cena e shot com herança do estilo global e projeto schema 11.
7. [x] Carregamento de fontes externas e presets visuais nomeados reutilizáveis, com prévia de paleta e integração ao guia da IA.
8. [x] Automação de ganho por pontos com easing na receita, projeto, prévia, exportação e editor.

Resultado esperado: produções com identidade visual e sonora consistente.

## FASE 4 — RECURSOS AVANÇADOS

Implementar:

1. [x] Vídeo local como camada visual editável.
2. [x] B-roll local decodificado na prévia e exportação.
3. [x] Alinhamento real de palavra em shots, beats, elementos, keyframes e legendas.
4. [x] Duração-alvo com aceleração segura e preservação aproximada de tom.
5. [x] Seleção múltipla e movimento conjunto de clipes na timeline com undo/redo atômico.
6. [x] Envelopes gráficos de áudio editáveis diretamente na timeline.
7. [x] Zoom temporal de 1× a 8× e navegação horizontal sincronizados com régua, shots, beats, camadas e áudio.
8. [x] Snapping geométrico de clipes visuais/áudio, movimento e trim de duração das pistas de áudio na timeline.
9. [x] Cache limitado e seek temporal para quadros de vídeo; a prévia de áudio reutiliza o seek nativo do Qt Multimedia.
10. [x] Trim de entrada, velocidade, duração de saída e recorte manual de vídeos locais.
11. [x] Áudio embutido em vídeos locais, com trim e velocidade sincronizados à camada e pista editável.
12. [x] Formatos de tela adicionais, incluindo projetos verticais 1080×1920 e Full HD 1920×1080.

Resultado esperado: ampliar as possibilidades criativas para produções mais elaboradas.

Ajuste a ordem se encontrar dependências técnicas importantes.

---

# PARTE 10 — TESTES OBRIGATÓRIOS

## 22. CRIAR UMA PRODUÇÃO REAL DE TESTE

Após implementar cada fase, crie uma produção demonstrativa válida.

Quero um vídeo de aproximadamente 90 segundos contando uma história curta.

Ele deverá testar, conforme as funcionalidades disponíveis:

- Narração em português brasileiro.
- Múltiplos shots na mesma cena.
- Cortes durante a narração.
- Visual Beats.
- Movimentação de câmera.
- Elementos animados.
- Grupos aninhados.
- Transições.
- Textos em destaque.
- Efeitos sonoros.
- Sincronização.
- Exportação.

Use um roteiro que tenha mudanças de composição suficientes para demonstrar os benefícios do novo motor.

**Não basta verificar se o JSON é aceito.**

É necessário testar se:

- O vídeo realmente renderiza.
- A narração permanece sincronizada.
- Os cortes acontecem corretamente.
- As animações não desaparecem prematuramente.
- As transições não causam frames pretos.
- As legendas acompanham o áudio.
- A timeline continua editável.
- O resultado exportado corresponde à prévia.
- A receita `examples/carros-caros-directors-cut-90s.production.json` gera uma montagem de 90 segundos com narração pt-BR, seis cenas, 18 shots, 20 Visual Beats, um efeito sonoro, câmera, transições e legendas. A exportação contém 2700 quadros; os cortes/beats são comparados à prévia e o projeto abre na interface.

Adicione testes automatizados sempre que possível, além da validação visual.

---

# PARTE 11 — ATUALIZAÇÃO DO GUIA PARA IA

## 23. ESSA ETAPA É OBRIGATÓRIA

Depois de implementar e testar as funcionalidades, atualize o arquivo **GUIA-PARA-IA.md** gerado pelo Studio Dark.

Esse arquivo será enviado a uma IA externa para que ela produza vídeos usando os recursos existentes.

Portanto, o guia precisa funcionar como uma especificação técnica precisa.

O guia atualizado deverá conter:

### A. Versões e capacidades

- Versão do formato de produção.
- Versão do projeto.
- Versão do catálogo.
- Resolução e FPS.
- Limites técnicos.
- Recursos disponíveis.

### B. Estrutura JSON completa

Documente cada campo aceito:

- Nome.
- Tipo.
- Finalidade.
- Valor padrão.
- Limites.
- Valores válidos.
- Campos obrigatórios.
- Incompatibilidades.

### C. Sincronização

Explique exatamente como a IA deve indicar:

- Duração-alvo.
- Cues.
- Progresso da fala.
- Duração dos shots.
- Eventos por palavra.
- Eventos por tempo.
- Transições.
- Keyframes.

### D. Elementos visuais

Liste todos os recursos realmente disponíveis:

- Tipos de elementos.
- Elementos nativos.
- Grupos.
- Presets.
- Arquivos de imagem.
- Arquivos de vídeo, se implementados.
- Animações.
- Movimentos de câmera.
- Tipografia.
- Efeitos.

### E. Biblioteca pessoal

Inclua somente os IDs válidos exportados pela instalação atual.

Não invente elementos ou arquivos.

### F. Exemplo de produção profissional

Inclua pelo menos um exemplo completo de JSON, válido e testado, demonstrando:

- Múltiplas cenas.
- Múltiplos shots.
- Visual Beats.
- Cortes.
- Movimento de câmera.
- Narração.
- Sincronização.
- Transições.
- Animações.

O exemplo precisa ser importável pelo aplicativo.

### G. Limitações

Liste claramente:

- O que já funciona.
- O que ainda não funciona.
- O que depende de arquivos locais.
- O que exige bibliotecas opcionais.
- O que não é compatível com formatos antigos.

**O guia não pode anunciar funcionalidades planejadas como se já estivessem implementadas.**

---

# PARTE 12 — CRITÉRIO FINAL DE SUCESSO

Meu objetivo final é conseguir fazer o seguinte:

1. Abrir uma conversa com o ChatGPT.
2. Enviar o guia atualizado do Studio Dark.
3. Pedir um vídeo de 1 minuto e 30 segundos sobre determinado assunto.
4. Informar o estilo desejado: dinâmico, cinematográfico, documental ou acelerado.
5. Receber um arquivo JSON completo com a direção audiovisual.
6. Importar o arquivo no Studio Dark.
7. Ter a narração gerada automaticamente.
8. Ter os planos, cortes, animações, legendas e efeitos sincronizados.
9. Visualizar uma montagem bem estruturada.
10. Fazer ajustes no editor e exportar o vídeo.

Quero que a IA consiga tomar decisões criativas de edição, enquanto o Studio Dark executa as instruções de maneira determinística e previsível.

Não quero depender de montar manualmente dezenas de cenas para cada vídeo.

Também não quero que o aplicativo invente decisões importantes contrárias ao planejamento explícito da IA.

O sistema deve combinar **liberdade criativa para a IA com execução técnica precisa pelo aplicativo**.

---

# INSTRUÇÃO FINAL AO DESENVOLVEDOR

Analise o código atual do Studio Dark e utilize este documento como especificação de evolução.

Identifique as alterações necessárias e implemente os recursos progressivamente.

Priorize o sistema de shots, Visual Beats e sincronização temporal, pois essas funcionalidades são a base de todo o restante.

A cada fase:

1. Implemente as alterações no código real.
2. Integre as funcionalidades ao aplicativo.
3. Execute os testes disponíveis.
4. Corrija regressões.
5. Gere uma produção demonstrativa.
6. Informe quais recursos estão totalmente funcionais, parcialmente funcionais ou pendentes.

Ao finalizar as etapas implementadas, atualize o gerador do **GUIA-PARA-IA.md** para documentar fielmente as capacidades reais.

O resultado desejado é transformar o Studio Dark em um sistema capaz de receber a direção de uma IA e produzir vídeos com montagem dinâmica, narração sincronizada, animações, câmera, tipografia, transições e áudio, mantendo a possibilidade de edição manual.

**A prioridade máxima é a qualidade da montagem final, a precisão da sincronização, a estabilidade do aplicativo e a capacidade de uma IA externa controlar a produção por JSON.**

Não quero apenas adicionar funcionalidades isoladas. Quero que todas elas trabalhem juntas como um motor de produção audiovisual coerente.
