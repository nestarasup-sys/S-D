# ADR 0034 — Premium Factory, slots de receita e enquadramento de câmera

Data: 27/10/2026. Estado: implementado em `teste-biblioteca`, revisão da Etapa 14.

## Contexto

A Premium Factory de código reutilizava nós, papéis, slots e animações, mas criar um item novo ainda exigia um construtor C++ exclusivo. A paleta JSON permitia direção de cor, porém o conteúdo dos slots era editado somente depois da inserção. Trajetórias de câmera eram montadas manualmente, o que deixava erros de staging fáceis de produzir mesmo quando o compositor executava corretamente os keyframes.

## Decisões

### Premium declarativo

- Os sete premiums anteriores mantêm IDs, manifestos v1 e seus construtores registrados. Não são migrados nem reconstruídos por outro caminho.
- Manifestos oficiais v2 podem declarar `definition.definitionVersion: 1`, com variantes `landscape` (canvas de referência 1280×720) e `portrait` (1080×1920). As variantes precisam preservar IDs, pais, componentes e papéis semânticos.
- O builder genérico converte camadas ordenadas em nós normais: `group`, `panel`, `rule`, `badge` e `text`. A definição também contém layout, conteúdo, estilo, bindings de tokens e presets limitados (`fade-rise`, `slide-left/right/up/down`, `pop-scale`, `soft-float`, `line-draw`, `stagger-reveal`). Animações viram keyframes comuns e preservam a avaliação temporal e o QPainter existentes.
- `question-hook-01.manifest.json` e `statistic-callout-pack-01.manifest.json` demonstram a criação de novos templates sem funções por ID. O CMake incorpora manifestos `assets/premium/*.manifest.json`; inicialização valida campos desconhecidos, canvases, bounds, tipos, IDs, hierarquia/ciclos, presets e correspondência entre metadados declarados e árvore.
- Manifestos são dados, não programas. Não há scripts, execução de comandos, plugins externos, caminhos de arquivo declarados nem instalação de pacotes de terceiros. A versão de definição é independente de `manifestVersion`; versões desconhecidas não são executadas.
- As definições declarativas são instaladas no registro premium pela inicialização de catálogo do módulo Storage e instanciadas por `makePremiumAsset`. O editor, thumbnails, previews, projeto persistido e exportação continuam usando os nós e o compositor atuais.

### Slots de receita e paletas

- Referências `kind: "library"` a premium oficial em receitas v2/v3 aceitam `slots` e `palette` opcionais simultaneamente.
- `slots` é um objeto de `id-curto-do-manifesto` para valor: texto para slot `text`, caminho local relativo ao JSON ou absoluto para slot `image`. O parser resolve somente IDs declarados pelo manifesto e os mapeia a camadas registradas; a IA não pode direcionar um ID arbitrário de nó interno.
- Texto respeita `maxLength` do manifesto quando definido e os limites globais existentes. Imagem aceita PNG/JPEG/WebP/BMP, caminho existente e canônico, decodificação válida, até 32 MiB e 40 megapixels. URLs, traversal e symlinks que escapem da receita são rejeitados. Só o slot de tela do Smartphone aceita imagem nesta versão; nenhum premium aceita vídeo ou ícone.
- `slots` altera conteúdo da instância, preservando IDs remapeados, parent/child, animações e demais atributos. `palette` aplica apenas tokens autorizados pelo manifesto. Ambas as operações são determinísticas e independentes; projetos antigos sem esses campos permanecem iguais.
- Antes da compilação no worker, imagem de slot é copiada por conteúdo para a mídia gerenciada do workspace e o nó do projeto aponta para a cópia. O projeto continua referindo-se a esse workspace; não afirmamos que a cópia seja um pacote portátil entre computadores.
- `premiumAssets[].slots` permanece a lista de papéis semânticos para compatibilidade. `premiumAssets[].slotDefinitions` adiciona o contrato executável (ID curto, tipo, nó, papel, opcionalidade e máximo de texto) para geração confiável por IA. O guia e o catálogo são gerados de manifestos embutidos, não de exemplos escritos manualmente.

### Direção de câmera e revelações

- `camera.path` é uma opção em `shots[].camera` da receita v3. Aceita `safeMargin`, `captionSafeBottom` e uma sequência de 2–24 `{target, at, padding, easing, reveal}`.
- `target` deve ser um ID de elemento ou grupo do próprio shot. `at` usa referência temporal existente baseada na fala medida: cue com `cueProgress`/`progress`, `offsetSeconds` ou `wordIndex` quando houver alinhamento real. O primeiro alvo deve ocorrer no corte; os tempos seguintes precisam crescer dentro do shot.
- O compilador transforma os bounds do nó por sua hierarquia de grupos e keyframes no frame indicado; centraliza o alvo e calcula zoom com o padding e margens seguras, incluindo a zona inferior das legendas. Ponto de interesse e movimento resultam em `CameraKeyframe`s editáveis. Não existe tracking por visão computacional nem rearranjo automático dos elementos.
- Revelação `spatial` exige objeto efetivamente visível no instante do ponto e rejeita `show` para esse alvo: o item entra no quadro pela trajetória. `temporal`/`combined` exigem opacidade efetiva zero no início e um beat `show` declarado; o primeiro usa esse evento independentemente do percurso, o segundo sincroniza revelação e percurso. Objetos com animação de entrada que os mantenha transparentes no ponto não satisfazem a validação espacial.
- `camera.path` é mutuamente exclusivo com `focus`, `focus.follow`, `cameraPreset` e beats de câmera no mesmo shot. Easing e avaliação continuam sendo os sistemas atuais. Câmeras preexistentes e receitas sem `camera.path` conservam semântica anterior.

## Capacidades e limites atuais

Há nove premiums registrados: Smartphone, Title Reveal, Comparison Pack, Dramatic Quote, Lower Third Editorial, Source Label, Number Emphasis, Question Hook e Statistic Callout Pack. Os dois últimos são declarativos v2. Os demais 21 itens do catálogo seguem planejados.

Slots textuais por receita estão disponíveis nos nove recursos segundo seus manifestos. A substituição de imagem está disponível somente para `smartphone:screen`. Nenhum premium aceita slot de vídeo ou ícone. Paleta limitada por token é aceita nos sete legados e nos dois declarativos. Templates declarativos suportam os componentes e animações listados acima; elementos vetoriais arbitrários, SVGs, mídia, constraints, scripts e dependências externas não fazem parte do formato.

`camera.path` dá assistência de enquadramento opcional por IDs, grupos, margens e tempos ligados à voz; não valida composição estética completa, não mantém títulos fixos ao viewport e não decide que o próximo objeto deve ficar oculto. Essa intenção precisa ser escolhida por `reveal`.

## Validação

Os exemplos em `examples/` são executados pelo parser e compilador de produção. As fixtures de câmera verificam keyframes, ausência do segundo alvo no primeiro quadro, entrada espacial durante o trajeto e estado de opacidade antes/depois do beat combinado. O teste de slots confere instâncias independentes, paletas simultâneas, mídia local, paths maliciosos, serialização/reabertura e cópia gerenciada. O teste QPainter de templates cobre nove recursos em 1280×720, 1920×1080 e 1080×1920.

## Consequências

Recursos editoriais simples podem ser criados por uma definição validada, sem um ramo `if assetId` nem um construtor exclusivo. Estruturas visuais mais específicas continuam podendo usar código legado até que uma necessidade comum justifique ampliar o conjunto de componentes declarativos. O contrato de receita aumenta de modo opcional; a aplicação rejeita conteúdo não suportado com erros ligados ao ID do recurso/slot. QPainter permanece como caminho oficial e a GPU experimental continua fora do fluxo de edição e exportação.
