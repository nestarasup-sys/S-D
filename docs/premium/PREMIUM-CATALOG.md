# Biblioteca premium — catálogo de 30 recursos

Estado revisado em 27/10/2026. “Implementado” significa que o recurso está registrado na biblioteca, possui camadas verificadas e pode ser editado no compositor QPainter. As demonstrações de receita são validadas separadamente; nenhum item planejado aparece como disponível.

| # | ID | Família | Estado |
|---|---|---|---|
| 1 | `premium:smartphone-premium-01` | Technology | Implementado: camadas editáveis, slots de texto/imagem, cores via JSON e layouts paisagem/retrato |
| 2 | `laptop-premium-01` | Technology | Planejado |
| 3 | `server-rack-premium-01` | Technology | Planejado |
| 4 | `ai-chip-premium-01` | Technology | Planejado |
| 5 | `dashboard-screen-premium-01` | Technology | Planejado |
| 6 | `newsroom-premium-01` | Documentary Scenes | Planejado |
| 7 | `office-premium-01` | Documentary Scenes | Planejado |
| 8 | `financial-district-premium-01` | Documentary Scenes | Planejado |
| 9 | `laboratory-premium-01` | Documentary Scenes | Planejado |
| 10 | `dark-city-premium-01` | Documentary Scenes | Planejado |
| 11 | `graph-pack-01` | Motion Graphics | Planejado |
| 12 | `timeline-pack-01` | Motion Graphics | Planejado |
| 13 | `premium:comparison-pack-01` | Motion Graphics | Implementado: dois painéis responsivos, seis slots textuais, animação e cores via JSON |
| 14 | `flowchart-pack-01` | Motion Graphics | Planejado |
| 15 | `premium:statistic-callout-pack-01` | Motion Graphics | Implementado por manifesto declarativo v2: valor, unidade, descrição e fonte via slots JSON, gráfico ilustrativo e layouts paisagem/retrato |
| 16 | `cause-effect-pack-01` | Motion Graphics | Planejado |
| 17 | `film-grain-overlay-01` | Editorial Overlays | Planejado |
| 18 | `light-streak-overlay-01` | Editorial Overlays | Planejado |
| 19 | `scanline-overlay-01` | Editorial Overlays | Planejado |
| 20 | `vignette-overlay-01` | Editorial Overlays | Planejado |
| 21 | `lens-glow-overlay-01` | Editorial Overlays | Planejado |
| 22 | `premium:lower-third-editorial-01` | Editorial Overlays | Implementado: título, subtítulo e fonte editáveis, cores via JSON |
| 23 | `premium:source-label-01` | Editorial Overlays | Implementado: identificação e detalhe editáveis, cores via JSON |
| 24 | `premium:title-reveal-01` | Kinetic Typography | Implementado: wipe QPainter, três slots textuais e paleta editorial |
| 25 | `premium:dramatic-quote-01` | Kinetic Typography | Implementado: frase, atribuição e fonte editáveis, cores via JSON |
| 26 | `punch-word-01` | Kinetic Typography | Planejado |
| 27 | `side-caption-01` | Kinetic Typography | Planejado |
| 28 | `stacked-headline-01` | Kinetic Typography | Planejado |
| 29 | `premium:number-emphasis-01` | Kinetic Typography | Implementado: valor, unidade, descrição e fonte editáveis, cores via JSON |
| 30 | `premium:question-hook-01` | Kinetic Typography | Implementado por manifesto declarativo v2: pergunta/contexto via slots JSON, animação tipográfica e layouts paisagem/retrato |

**Estado do registro:** 9 recursos implementados e 21 planejados. Sete recursos legados usam manifestos v1 e construtores C++ registrados; Question Hook e Statistic Callout usam manifestos v2 com definições visuais declarativas. O builder genérico instancia camadas `group`, `panel`, `rule`, `badge` e `text`, aplica layouts responsivos e converte presets permitidos em keyframes normais. As definições são dados validados, não código executável. A aplicação não instala pacotes premium externos.

Os 11 filtros temáticos existentes continuam preservados. Slots de texto podem ser preenchidos pela receita em qualquer dos nove premium; a troca de imagem via receita é suportada no slot `screen` do Smartphone. Os demais slots são textuais. Nenhum pacote premium anuncia slot de vídeo ou ícone. `premiumAssets[].slotDefinitions` no catálogo de capacidades informa IDs, tipos, nós, papéis, opcionais e tamanhos quando aplicáveis. Paletas e slots podem estar juntos na mesma instância. Cores são limitadas aos tokens declarados por manifesto. As imagens selecionadas são copiadas para mídia gerenciada do workspace; isso não empacota o projeto para outro computador. A inserção usa os nós normais e QPainter; o protótipo GPU não é requisito.

Exemplos de composição, slots, travelling espacial/temporal e paletas ficam em `examples/`. `camera.path` opcional enquadra IDs de alvos em ordem, respeita bounds transformados de grupos e margens de legenda, e gera keyframes editáveis. A revelação espacial mantém o objeto visível fora do enquadramento; a temporal exige opacidade inicial zero e Visual Beat `show`; a combinada usa ambos. Evidências visuais temporárias são gravadas em `artifacts/` e não fazem parte do catálogo distribuído.
