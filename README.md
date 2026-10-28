# Studio Dark · Native Lab

Protótipo 0.3 em C++20 e Qt Quick/QML: projetos e biblioteca organizados, roteiro externo → direção por JSON → voz local → projeto editável → MP4 com áudio.

## Abrir

Clique em **ABRIR-STUDIO.bat**. As ferramentas locais ficam em `.tools/` e os binários em `build/`. O BAT configura os caminhos necessários e abre a versão compilada ativa sem terminal permanente. Não execute apenas o EXE fora desse ambiente de desenvolvimento. Se o editor estiver aberto durante uma compilação, o build usa uma pasta nova e preserva a sessão existente; a próxima abertura usa a versão nova.

Em outra máquina Windows x64 com Python 3.12 e internet, use **PREPARAR-E-COMPILAR.bat**. Downloads não exigem instalação global de Qt ou C++ e ficam na pasta do projeto.

Para gerar o primeiro episódio completo, clique em **CRIAR-VIDEO-CARROS.bat**. Ele cria uma pasta nova em `artifacts/carros-AAAAMMDD-HHMMSS`, gera voz/legendas/cenas, exporta o vídeo e abre o projeto no editor. É necessária uma voz pt-BR instalada no Windows.

## Tela inicial de projetos

O aplicativo abre em **Projetos**, com cartões, prévias, busca e atalhos para a biblioteca. **Novo projeto** pede um nome e formato — paisagem 1280×720, paisagem Full HD 1920×1080 ou retrato 1080×1920 — e salva a composição vazia na pasta de dados do aplicativo. **Importar projeto ou JSON** abre qualquer arquivo `.json` (projetos `.studio.json` e receitas `.production.json`); **Importar receita IA** abre diretamente o importador de produção.

Cada cartão oferece **Abrir**, **Duplicar**, **Renomear** e **Excluir**. A exclusão move o registro para a **Lixeira**, de onde pode ser restaurado; os arquivos do projeto e suas mídias são mantidos. Projetos gerados anteriormente na pasta de produções do app são reconhecidos na primeira inicialização da coleção.

## Biblioteca completa

O botão **Biblioteca completa** está sempre no topo e também no painel do editor. A tela reúne objetos nativos, cenários, personagens, dados, imagens e variações personalizadas.

- **Adicionar arquivo** importa PNG/JPEG/WebP/BMP ou SVG, com nome e categoria. O app guarda uma cópia própria da mídia.
- **Salvar seleção do editor** transforma o objeto selecionado em um item reutilizável, preservando estilo, pose, tamanho e movimento. Imagens/SVG também têm sua mídia copiada.
- **Organizar** renomeia ou muda a categoria de um item. **Usar** adiciona uma nova instância ao projeto aberto, com identidade independente.
- Categorias podem ser criadas, renomeadas e excluídas. Ao excluir uma categoria, seus itens vão para **Sem categoria**.
- **Excluir** remove o item da coleção; ele pode ser restaurado em **Excluídos / restaurar**. Nativos continuam disponíveis ao motor de renderização e as mídias importadas são retidas, preservando projetos já criados.

As lixeiras são reversíveis e não fazem limpeza física de arquivos. Não há purga permanente neste corte. A organização da biblioteca é local e não altera o catálogo de capacidades nativas do guia de produção.

## Seu roteiro com uma IA externa

1. Escreva seu roteiro em qualquer lugar.
2. Use o botão **Guia IA** para salvar o guia atual com os itens ativos da sua biblioteca pessoal. A cópia do repositório lista as capacidades nativas e explica como salvar um manifesto local.
3. Envie à IA o roteiro e o guia gerado; peça uma receita v3 quando quiser cortes/ações visuais dentro da mesma cena, ou v2 para uma composição tradicional, e salve como `.production.json`.
4. No editor, use **Importar produção IA**; na tela inicial, use **Importar receita IA** ou selecione qualquer `.json` em **Importar projeto ou JSON**. O app valida o plano, gera a voz local, mede os tempos e monta as cenas.
5. Reproduza, ajuste os objetos e exporte MP4.

Receita v2 aceita composições sem títulos ou cenários impostos, grupos, keyframes ligados às falas, câmera e referências por ID a itens pessoais ativos. Receitas v1 antigas continuam funcionando. Exemplos: [roteiro sobre carros](examples/ROTEIRO-CARROS-CAROS.md), [receita v1 de carros](examples/carros-caros.production.json) e [composição livre v2](examples/composicao-livre-v2.production.json).

Exemplo de produção v3 longa: [Antes do primeiro gole](examples/antes-do-primeiro-gole.production.json), com [guia de direção e fontes](docs/ANTES-DO-PRIMEIRO-GOLE.md). A pasta `examples/media/stage16/` contém artes, voz auxiliar e um clipe original claramente ilustrativo. O MP4, projeto editável, quadros e relatório local não são versionados; consulte `artifacts/antes-do-primeiro-gole-stage16/` depois de executar a receita neste ambiente.

## Shots e Visual Beats (receita v3)

Receitas v3 aceitam `canvas` opcional: `{ "width": 1080, "height": 1920 }` para retrato ou `{ "width": 1920, "height": 1080 }` para paisagem Full HD. Sem esse objeto, permanece 1280×720. Somente esses três tamanhos são aceitos. O importador usa as dimensões escolhidas para centro padrão da câmera, legendas, prévia e exportação; cenários nativos 16:9 preservam a proporção e são cortados nas laterais em retrato. Projetos horizontais antigos não mudam. Exemplo testável: [produção vertical 1080×1920](examples/retrato-1080x1920-v3.production.json).

Uma cena pode ter vários shots sem quebrar a narração. O início de cada plano é resolvido contra a fala sintetizada; o próximo início encerra o plano anterior, e os planos precisam cobrir a cena sem lacunas. Cada shot tem composição e estado inicial de câmera próprios. `camera.focus` pode enquadrar um elemento ou grupo do próprio shot com margem automática; `follow: true` acompanha o alvo quadro a quadro. `transitionIn` aceita corte seco, fade, dissolve, slide horizontal/vertical e zoom com duração configurável em quadros. No editor, selecione um quadro dentro do shot e ajuste tipo, duração e direção da transição de entrada no inspetor; o primeiro shot da cena permanece sem transição.

Em uma receita v3, `scenes[].title` é o nome da cena na timeline/projeto. Ele não é repetido como texto visual em cada shot; para títulos na imagem, inclua elementos de texto nos próprios shots.

Receitas v3 aceitam `artDirection` global e overrides parciais em cena e shot. A herança combina os valores na ordem global → cena → shot; `color`, `secondaryColor` e `fontSize` explícitos no elemento prevalecem. O inspetor permite editar o escopo global ou criar/alterar o estilo da cena/shot atual. A composição e a exportação usam o mesmo estilo resolvido. Projetos com overrides locais usam schema 11; versões anteriores continuam aceitas, e projetos schema 1–13 abrem. Fontes externas e presets visuais nomeados ainda estão pendentes. Veja a [decisão de arquitetura 0016](docs/architecture/0016-scene-shot-art-direction.md).

No inspetor, **Direção de arte global** edita as cores, a fonte, a tipografia, a margem segura e a posição das legendas. **Intensidade de edição** aplica à cena atual um perfil de entrada/saída entre 0 e 100; a ação inteira pode ser desfeita de uma vez. Ajustes finos continuam disponíveis por objeto.

Receitas v3 aceitam `kind: "video"` com caminho local em `source` para inserir vídeo/B-roll como camada editável. `video.sourceInSeconds` define o trim de entrada; `video.durationFrames` define a saída na timeline; `video.speed` altera a velocidade entre 0,25× e 4×; `video.crop` define um recorte normalizado da fonte. O mesmo FFmpeg compositor aplica os ajustes na prévia e no MP4 exportado. O inspetor também permite editar esses valores. Formatos aceitos: MP4, MOV, MKV, WebM, AVI e M4V. A máquina precisa ter acesso à fonte e ao FFmpeg configurado pelo app. Camadas simples usam schema 5; controles avançados usam schema 10. Veja [a decisão de arquitetura 0014](docs/architecture/0014-video-layers.md).

Projetos schema 4–13 e receitas v3 aceitam até 32 pistas de áudio independentes, incluindo efeitos disparados por Visual Beat e o áudio de vídeos locais. Selecione uma camada de vídeo e use **Adicionar áudio do vídeo à timeline**; a nova pista segue o clipe quando ele é movido, aparado ou tem trim/velocidade alterados. Ajustes manuais de início, duração, trim ou velocidade na pista a desvinculam para edição independente. Na timeline, ajuste início/duração/ganho, trim de fonte/velocidade e fades, ligue ducking e edite automação de ganho visualmente: clique na linha para criar um ponto e arraste para mudar o tempo e o ganho; a curva pode usar easing linear ou suavizado. Com WAV de voz PCM mono a 24 kHz, prévia e exportação aplicam a mesma envoltória RMS por quadro; outros formatos usam sidechain FFmpeg na exportação. Receitas podem apontar para áudios locais ou vídeos locais com faixa de áudio, relativos ao JSON ou absolutos; sincronizam o início com fala/progresso/palavra e aceitam `sourceInSeconds` (0–3600, padrão 0) e `playbackRate` (0,25–4, padrão 1), além de fades e pontos relativos ao início da faixa. Fades não nulos usam schema 6 ou superior; automação usa schema 8; efeitos de beat usam schema 9; trim/velocidade de áudio usa schema 13. Veja o guia da IA e as decisões de arquitetura [0013](docs/architecture/0013-multitrack-audio.md), [0015](docs/architecture/0015-visual-beat-sound-effects.md), [0019](docs/architecture/0019-graphical-audio-envelopes.md) e [0023](docs/architecture/0023-embedded-video-audio.md).

Receitas v3 podem definir `wordAnimation` na raiz para animar cada palavra da legenda com alinhamento real: `highlight`, `none`, `pop`, `rise`, `fade` ou `pop-rise`. O inspetor permite editar preset, escala inicial, deslocamento e duração da palavra; a prévia e o render usam o mesmo tempo. Projetos schema 12 persistem essas opções; projetos v1–v11 continuam abrindo.

Receitas v3 aceitam `editing.intensity` de 0 a 100 para escolher entradas e saídas padrão apenas quando a IA não especificou movimento ou duração. Sem esse parâmetro, o padrão antigo é mantido.

Visual Beats podem revelar/esconder elementos, alterar transformações, animar a câmera ou disparar um efeito sonoro local. O som vira uma pista editável ligada ao beat; a fonte precisa existir junto ao JSON ou estar em caminho absoluto. Beats, início de elementos e keyframes aceitam `wordIndex` com timestamp real emitido pelo sintetizador pt-BR local. As legendas visuais também destacam a palavra falada no quadro correspondente, usando a cor primária da direção de arte; no inspetor, é possível ajustar os tempos de início e fim por palavra em quadros, e na faixa da legenda é possível arrastar os limites com precisão de um quadro. Limites compartilhados movem as palavras vizinhas em conjunto e cada gesto pode ser desfeito/refeito. Os tempos são salvos no projeto schema 7; efeitos de beat usam schema 9. Os eventos são convertidos em quadros/keyframes ou faixas de áudio do projeto, preservando a edição manual. A timeline exibe os shots e os eventos; alças de corte movem os planos e seus conteúdos relacionados como um único gesto de desfazer. Projetos anteriores continuam abrindo; vídeo requer schema 5 ou superior e fades usam schema 6 ou superior.

O guia lista os campos, limites e restrições da v3. Exemplo completo: [shots e Visual Beats](examples/shots-visual-beats-v3.production.json). A síntese pt-BR grava limites de palavra e pode usá-los em `wordIndex`; confira a [decisão de arquitetura 0012](docs/architecture/0012-word-aligned-beats.md). Veja também a [decisão de arquitetura 0009](docs/architecture/0009-shots-and-visual-beats.md).

O smoke de produção inclui o [Director's Cut de 90 segundos](examples/carros-caros-directors-cut-90s.production.json): seis cenas, 18 shots, 20 Visual Beats, um efeito sonoro local e direção por palavra. O teste gera a voz pt-BR, exporta 2700 quadros, compara 36 quadros de corte/beat entre prévia e MP4 e abre o projeto no editor. Copie `examples/media/soft-chime.wav` para uma pasta `media` ao lado de uma cópia da receita. `scripts/update-ai-guide.ps1` incorpora esse mesmo JSON no guia para IA.

## Duração fixa e sincronização proporcional à fala

A receita IA v2 pode controlar a duração final usando `"timingMode":"fit-to-duration"`, `"targetDurationSeconds":90` e `"preserveVoicePitch":true`. O aplicativo sintetiza a voz e depois calcula os quadros a 30 FPS. Aceita alvos opcionais por cena e por fala e combina aceleração limitada a 15% (FFmpeg `atempo`, preservando aproximadamente o tom) com pausas. Quando a narração não cabe, recusa a produção e explica o problema em vez de alterar o texto. Pausas planejadas superiores a 3 segundos após uma fala ou superiores a 5 segundos ao final de uma cena geram avisos de ritmo nos logs e no relatório. O relatório de sucesso registra ainda as janelas de fala e o fator de velocidade calculado.

Objetos, keyframes, shots, beats, faixas e legendas podem usar `wordIndex` quando a síntese pt-BR fornece alinhamento real. Os índices seguem a base zero e são resolvidos para quadros, preservando a edição no projeto. `cueProgress` de 0 a 1 e `offsetSeconds` continuam disponíveis como alternativas. O projeto tem duração exata em quadros, por exemplo 90 s = 2700 quadros; metadados do MP4/AAC podem refletir atraso de codificação. Consulte o [guia da IA](docs/GUIA-PARA-IA.md), a ADR 0008 e as decisões de sincronização por palavra.

## Fundação de composição livre (schema v3)

O editor já permite envolver o elemento selecionado em um grupo, gravar keyframes no playhead, escolher curvas linear/ease-in/ease-out/ease-in-out no inspetor e animar centro, zoom e rotação da câmera. Prévia e render usam o mesmo cálculo por frame; projetos v1/v2 continuam abrindo. O projeto de demonstração usa um grupo e movimentos de câmera para servir como prova visual.

Grupos podem ser selecionados pela lista de camadas; arrastar sua faixa move o grupo, seus filhos e os keyframes juntos. A timeline rola até a faixa do objeto selecionado; a lista de pistas de áudio tem rolagem própria para manter espaço para clipes visuais. Na timeline, use Ctrl+clique para selecionar vários clipes e arraste um deles para mover o conjunto, mantendo os espaçamentos e os limites de cada cena em uma única ação de desfazer. O **Snap** opcional alinha bordas dos clipes visuais e de áudio ao playhead, cenas, shots, beats, outras camadas e outras pistas; a guia âmbar acompanha o zoom. A alça ↔ no início do envelope move a pista mantendo sua duração; a alça direita ajusta o fim por frame, e ambas mantêm o beat de som vinculado. O zoom temporal de 1× a 8× mantém régua, shots, camadas e envelopes alinhados; use Ctrl+roda sobre a régua para aproximar no ponto do ponteiro, Shift+roda ou a barra horizontal para navegar. Excluir um grupo remove sua árvore. Grupos inteiros podem ser salvos e reutilizados como presets, inclusive pelo JSON v2 com `kind: library` e um ID do manifesto pessoal. Curvas de keyframes lineares e suavizadas podem ser escolhidas no inspetor para a câmera e os objetos. Conectores externos continuam pendentes. Receitas v2 e v3 estão disponíveis; consulte as [decisões de arquitetura 0004](docs/architecture/0004-groups-camera-keyframes.md), [0009](docs/architecture/0009-shots-and-visual-beats.md), [0018](docs/architecture/0018-timeline-multi-selection.md), [0020](docs/architecture/0020-timeline-zoom-navigation.md), [0021](docs/architecture/0021-manual-caption-word-timing.md), [0022](docs/architecture/0022-timeline-geometric-snapping.md), [0023](docs/architecture/0023-embedded-video-audio.md), [0024](docs/architecture/0024-canvas-formats.md), [0025](docs/architecture/0025-video-frame-cache-seek.md) e [0026](docs/architecture/0026-reusable-camera-presets.md), além da [direção futura](FUTURO-DO-APP.md).

## Diagnóstico de importação

O botão **Logs** no topo abre os arquivos locais de diagnóstico de importações IA. Cada tentativa gera um arquivo `production-*.jsonl` com data, etapas, mensagens de validação e códigos de término, mesmo quando o JSON é rejeitado antes de iniciar a voz. Os 30 logs mais recentes são mantidos; não são enviados automaticamente, e o conteúdo integral do roteiro/JSON não é registrado. Mensagens de erro podem conter caminhos ou identificadores locais; revise os logs antes de compartilhá-los. Para relatar uma falha, compartilhe o `.jsonl` correspondente e, se puder, o `.production.json` com dados pessoais removidos. Os logs são apenas da importação/produção IA, não de todos os eventos da interface.

## Testar pela interface

1. A demonstração começa com texto e forma animados em uma composição de 12 segundos.
2. Reproduza, navegue pela régua e selecione um objeto na biblioteca ou na prévia.
3. Arraste o objeto na prévia ou ajuste posição, dimensões, texto, cor e tempo no inspetor.
4. Adicione texto, forma, PNG/SVG, vídeo local ou um dos 72 recursos nativos, incluindo tecnologia, gráficos e cenários documentais. Ajuste pose, roupa/pele, movimento, giro e deslocamento. Mova clipes pela timeline com Snap opcional, alinhando às referências visuais e de áudio; para liberar espaço, reduza a duração pelo inspetor.
5. Use desfazer/refazer, salve com um nome novo e reabra o projeto.
6. Em uma produção gerada, escolha a cena no seletor abaixo da prévia. A biblioteca e a timeline mostram os objetos daquela cena. O áudio permanece fixo enquanto você ajusta visuais.
7. Exporte MP4 com nome novo. Produções geradas incluem narração, trilha e efeitos; projetos visuais sem master de áudio continuam exportando sem som.

Atalhos: `Ctrl+S` salva, `Ctrl+O` abre, `Ctrl+Z` desfaz e `Ctrl+Shift+Z` refaz. Campos numéricos e de cor aplicam a alteração ao perder o foco; o conteúdo do texto é atualizado durante a digitação. Navegar pela régua pausa a reprodução. A recuperação automática fica disponível pelo botão na biblioteca após uma edição.

## Estrutura para desenvolvimento

```text
apps/
  desktop/          aplicação Qt, ponte C++ e painéis QML
  render-worker/    processo de renderização e codificação
  production-worker/ validação, voz SAPI e montagem automática
modules/
  core/             documento e edição, C++ sem Qt
  storage/          JSON, lock, revisões e escrita atômica
  composition/      composição compartilhada entre prévia e exportação
  media/            PCM/WAV e trilha procedural
  production/       receita, catálogo, guia e compilador audiovisual
examples/           roteiro sintético e receita reproduzível
scripts/            preparo, ambiente, build, execução e smoke audiovisual
tests/              comandos, tempo, persistência e composição
docs/architecture/  decisões e limites do protótipo
.tools/             SDK e compilador locais; fora do Git
build/              binários e arquivos de compilação; fora do Git
artifacts/          capturas, MP4s e relatórios de teste; fora do Git
```

Comandos PowerShell para desenvolvimento:

```powershell
.\scripts\setup.ps1
.\scripts\build.ps1 -Test
.\scripts\smoke.ps1
.\scripts\smoke-production.ps1
.\scripts\smoke-workspace.ps1
.\scripts\update-ai-guide.ps1
.\scripts\run.ps1
```

Para medir o backend raster e o protótipo Qt Quick Scene Graph nos três canvases, execute `scripts/build.ps1` e `scripts/benchmark-renderers.ps1`. O benchmark cobre primitivas em 20, 120 e 200 elementos, cenas híbridas estáticas/animadas, três resoluções, o atlas dos 72 nativos e pressão de cache; grava imagens douradas, diferenças e métricas em `artifacts/renderer-benchmark-*`. O Scene Graph consome o `RenderSnapshot`, desenha primitivas vetoriais com QSG e mantém a ordem das camadas; texto, legendas, nativos e mídia raster/SVG/WebP são rasterizados por `CompositionPainter` em CPU e reutilizados como texturas GPU. Um grafo de composição interno planeja grupos e pilhas de efeitos experimentais, sem mudar o formato de projeto. `QQuickRhiItem` executa máscara alfa, blur separável e glow em Direct3D11, inclusive em cenas híbridas; `scripts/test-shaders.ps1` e `scripts/benchmark-effects.ps1` validam imagens, diferenças e tempos em `artifacts/gpu-effects-test` e `artifacts/effect-benchmark-*`. A leitura de retorno GPU é diagnóstica; timestamp QRhi mede o quadro GPU completo quando disponível, não cada passe. `scripts/benchmark-renderers.ps1` também registra o adaptador ativo do QRhi e leituras DXGI LOCAL/NON_LOCAL antes, durante, antes da liberação e após pedir ao Qt para liberar os recursos. A estimativa de texturas, a RAM do processo e o uso DXGI do processo são grandezas separadas; o uso DXGI não é a soma exata das texturas do Studio Dark. No computador de desenvolvimento, o adaptador ativo reportado foi AMD Radeon RX 7600 em D3D11. O editor e a exportação continuam no QPainter, sem novos campos JSON. O backend Qt Quick Software segue sem suporte; vídeo e transições de shots seguem fora do protótipo. Consulte [ADR 0032 — motor gráfico 2.0](docs/architecture/0032-render-engine-2.md).

Leia a [arquitetura da produção por IA](docs/architecture/0002-ai-directed-production.md) e a [fundação nativa](docs/architecture/0001-native-prototype.md). O [plano futuro](FUTURO-DO-APP.md) mantém o estado atual e a visão de produto.

A persistência da coleção e os limites de exclusão estão na [arquitetura de projetos e biblioteca](docs/architecture/0003-projects-and-library.md).

A composição por grupos/câmera e a direção v2 estão em [0004](docs/architecture/0004-groups-camera-keyframes.md) e [0005](docs/architecture/0005-production-v2-personal-library.md). Os presets de grupos são detalhados na [ADR 0006](docs/architecture/0006-reusable-group-presets.md). Shots e Visual Beats estão em [0009](docs/architecture/0009-shots-and-visual-beats.md). A intensidade da edição e seus defaults estão em [0010](docs/architecture/0010-editing-intensity.md).

A evolução proposta a partir do vídeo de referência está em [Futuro do aplicativo](FUTURO-DO-APP.md): composição livre, grupos, câmera, documentos, vídeo e direção por IA. Esse documento é planejamento, não uma lista de recursos já implementados.

## Estado deste corte

Há biblioteca vetorial autoral, quatro poses de personagem, seis movimentos de entrada, PNG/SVG/vídeo local com trim, velocidade e recorte, áudio extraído de vídeo em pista editável, receitas v1/v2/v3, shots e transições editáveis, direção de arte global/por cena/shot, Visual Beats incluindo efeitos sonoros sincronizados, narração pt-BR local, sincronização e animação de palavra nas legendas, trilha/efeitos procedurais, até 32 pistas com ducking/fades/automação por pontos e easing, forma de onda e reprodução audiovisual. Continuam pendentes ferramentas avançadas de timeline. Projetos v1–v13 continuam abrindo, e as receitas v3 continuam compatíveis.

A IA externa prepara a receita; o app executa capacidades reais. Personagens são objetos parametrizados, sem edição individual dos ossos ou sincronização labial. SVG não é convertido automaticamente em partes nativas. Receitas v3 permitem definir e reutilizar trajetórias nomeadas de câmera; o compositor mantém até 64 MiB de quadros de vídeo em cache e usa seeks diretos em saltos longos, preservando o alinhamento da prévia com a exportação. A prévia de áudio usa o seek nativo do Qt Multimedia. Continuam pendentes automação de ritmo a partir do conteúdo, edição avançada de duração/overlap, curvas Bézier personalizadas, carregamento de fontes externas e presets visuais nomeados.

Alterar texto falado ou os volumes da trilha exige recompilar a receita. Editar uma legenda visual não regenera a voz. Imagens e áudio dependem dos caminhos locais; o projeto ainda não é um pacote portátil. Use uma janela do editor por vez. Instalador e redistribuição pública das dependências ficam para uma etapa própria.

### Escalabilidade de projetos

Receitas e projetos não possuem teto fixo agregado de nós nem corte por tamanho total de JSON/projeto. A leitura valida o conteúdo e a estrutura; o processamento continua limitado pela memória e pelo tempo disponíveis. Permanecem limites independentes para duração, cenas, shots, keyframes, pistas, profundidade de grupos e texto. A timeline usa `QAbstractListModel`/`ListView` virtualizada e indexa IDs para seleção; renderização e exportação ainda avaliam as camadas ativas do quadro inteiro, portanto documentos maiores custam mais CPU, RAM e tempo e não têm garantia de taxa de quadros. O benchmark estrutural e seus tempos/memória ficam em `artifacts/large-project-scaling/report.json`, reproduzido por `scripts/benchmark-large-projects.ps1`. Política e consequências: [ADR 0033 — escala de projetos grandes](docs/architecture/0033-large-project-scaling.md).


## Biblioteca documental expandida

O catálogo vetorial nativo reúne **72 itens** (14 originais e 58 novos), incluindo ilustrações de tecnologia, ciência, finanças, comunicação, gráficos ilustrativos e oito cenários adicionais. Os recursos novos são procedurais: não precisam de download, funcionam sem internet e são editáveis como objetos `kind: "native"` nas receitas v3.

Após compilar, os itens adicionais entram automaticamente em bibliotecas pessoais já existentes; os itens importados e as exclusões anteriores são preservados. A documentação de escolhas de câmera, tipografia e ritmo fica em [Direção de roteiro dinâmico](docs/DIRECAO-ROTEIRO-DINAMICO.md); o contrato técnico JSON continua em [Guia para IA](docs/GUIA-PARA-IA.md). Ilustrações não substituem imagens/B-roll reais nem representam estatísticas verificadas.

### Protótipo experimental de efeitos GPU

O protótipo cria um grafo de operações a partir do mesmo `RenderSnapshot`, preserva a ordem entre camadas vetoriais, texturas e itens `QQuickRhiItem`, e aceita pilhas ordenadas/repetidas de blur, glow e máscara. A cena de demonstração inclui grupos aninhados; o efeito externo do grupo roda no QRhi, enquanto efeitos em filhos dentro da superfície combinada ainda são rasterizados em CPU. O cache QSG tem orçamento global de 64 MiB. A infraestrutura segue experimental: não está ligada à prévia do editor nem à exportação FFmpeg, e efeitos não foram adicionados ao JSON/guia de IA. Detalhes, resultados visuais e limites estão na [ADR 0032](docs/architecture/0032-render-engine-2.md); `scripts/benchmark-effects.ps1` gera os relatórios locais.

Visual Beats `show`/`hide` são aplicados no quadro exato e preservam a opacidade visível; regressões de estado, QPainter e MP4 ficam em `scripts/test-visual-beats.ps1`. As receitas oficiais de [revelação sequencial](examples/visual-beats-sequential-reveal-v3.production.json) e [transformação com câmera](examples/visual-beats-transform-camera-v3.production.json) são parser-tested. Textos comuns pertencem ao espaço da cena e acompanham a câmera; legendas automáticas permanecem ancoradas ao viewport durante pan, zoom, rotação e transições. A distinção e a regressão visual estão na [ADR 0032](docs/architecture/0032-render-engine-2.md), no [guia técnico para IA](docs/GUIA-PARA-IA.md) e em [Direção de roteiro dinâmico](docs/DIRECAO-ROTEIRO-DINAMICO.md).
# Biblioteca premium e Premium Factory

O registro possui nove recursos oficiais editáveis pelo QPainter: `premium:smartphone-premium-01`, `premium:title-reveal-01`, `premium:comparison-pack-01`, `premium:dramatic-quote-01`, `premium:lower-third-editorial-01`, `premium:source-label-01`, `premium:number-emphasis-01`, `premium:question-hook-01` e `premium:statistic-callout-pack-01`. Sete recursos mantêm os construtores C++ e manifests v1; Question Hook e Statistic Callout usam manifestos v2 declarativos embutidos. Um builder genérico converte as camadas declaradas em nós normais, sem construtor específico por ID. As definições permitem componentes `group`, `panel`, `rule`, `badge` e `text`, variantes paisagem/retrato, tokens de cor, papéis semânticos e keyframes de presets permitidos. Não executam código ou scripts externos.

Receitas v2/v3 aceitam `slots` e `palette` no mesmo elemento premium. Os slots são IDs dos manifestos e substituem texto/imagem da instância sem remover hierarquia ou animação; o catálogo `docs/capabilities-v1.json` expõe `slotDefinitions` com tipo, limite e nó vinculado. Slots de texto existem nos nove premium. Smartphone aceita texto e imagem no slot `screen`; a mídia local é validada e copiada para armazenamento gerenciado do workspace. Essa cópia não empacota o projeto para outro computador. Nativos aceitam `palette.primary` e `palette.secondary`; templates premium aceitam apenas tokens registrados. Nenhum premium anuncia slots de vídeo/ícone. Placeholders de números e comparações não são fatos.

`camera.path` é um contrato opcional em shots v3. A receita lista IDs de alvo e tempos sincronizados à voz; o compilador mede os bounds através de grupos, calcula centro/zoom com margem e converte pontos em keyframes editáveis. `reveal: "spatial"` mantém o elemento visível na cena e o faz entrar pela câmera; `"temporal"` mantém `opacity: 0` até `show`; `"combined"` combina o beat com o travelling. As trajetórias são dirigidas, não tracking automático. Consulte os exemplos [de slots](examples/premium-dramatic-quote-slots-v2.production.json), [de tela do smartphone](examples/premium-smartphone-slots-v2.production.json), [de travelling espacial](examples/camera-path-spatial-v3.production.json) e [com Visual Beat](examples/camera-path-combined-v3.production.json), além de [Direção de roteiro dinâmico](docs/DIRECAO-ROTEIRO-DINAMICO.md), [Guia para IA](docs/GUIA-PARA-IA.md) e a [ADR 0034](docs/architecture/0034-premium-factory-and-palette-overrides.md). O catálogo mantém nove recursos implementados e 21 planejados.

Antes de exportar, **Qualidade visual** e **Exportar MP4** abrem uma análise consultiva dos quadros representativos. Ela procura texto premium que não cabe, texto cortado/muito próximo da borda, colisões entre textos e legendas, contraste mensurável sobre superfícies sólidas, beats `show` que não revelam o alvo e alvos `camera.path` mal enquadrados. A análise usa o compositor QPainter/RenderSnapshot, não altera o projeto e não bloqueia exportação. Seu relatório JSON fica em Logs como `visual-quality-last.json`; para um projeto salvo, `studio_render.exe --project projeto.studio.json --quality-report relatorio.json` faz a análise sem renderizar o vídeo. Intenção artística, fundos com imagem, fatos e estética geral ainda exigem revisão humana. Slots premium podem declarar a política de texto `shrink` no manifesto; ela reorganiza linhas e reduz a fonte até o mínimo declarado, preserva todas as palavras e reporta overflow se ainda não couber. Caminhos de imagem em slots de receita são relativos e confinados à pasta do JSON; isso não cria um pacote portátil entre computadores. Veja a [ADR 0035](docs/architecture/0035-visual-quality-and-premium-text-layout.md) e o [pacote de avaliação criativa para IA](docs/ai-evaluation/README.md).
