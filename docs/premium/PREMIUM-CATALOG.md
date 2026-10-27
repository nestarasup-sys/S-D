# Biblioteca premium — catálogo de 30 recursos

Estado revisado em 26/10/2026. “Implementado” significa que o recurso está registrado no aplicativo, possui manifesto conferido com seu construtor e pode ser editado no compositor QPainter. As demonstrações de receita são validadas separadamente; nenhum item planejado aparece como disponível.

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
| 15 | `statistic-callout-pack-01` | Motion Graphics | Planejado |
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
| 30 | `question-hook-01` | Kinetic Typography | Planejado |

**Estado do registro:** 7 recursos implementados e 23 planejados. Os 7 manifestos oficiais v1 são embutidos e verificados contra a árvore criada em C++; os manifestos descrevem metadados e não executam código. A fábrica interna fornece componentes e presets de animação a quatro dos construtores novos. Não existe ainda um interpretador de pacotes externos.

Os 11 filtros temáticos existentes continuam preservados. Os slots de imagem atualmente implementados pertencem ao Smartphone; os demais slots são texto. Nenhum pacote premium anuncia slot de vídeo ou ícone. Cores por JSON são restritas aos tokens declarados em cada manifesto. A inserção de recursos usa o mesmo grafo de nós e o compositor QPainter; o protótipo GPU não é requisito.

Exemplos de composição, travelling e paletas ficam em `examples/`. Evidências visuais temporárias são gravadas em `artifacts/` e não fazem parte do catálogo distribuído.
