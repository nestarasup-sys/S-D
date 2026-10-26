# Biblioteca premium — catálogo de 30 recursos

Status em 26/10/2026. “Implementado e validado” significa pacote oficial registrado, manifesto conferido contra o construtor, composição editável no QPainter e fluxo biblioteca/receita testado. Itens planejados não aparecem como disponíveis no app.

| # | ID | Família | Estado |
|---|---|---|---|
| 1 | `premium:smartphone-premium-01` | Technology | Implementado e validado: biblioteca, edição QPainter, slots de texto/imagem, receita IA e exportação MP4 |
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
| 13 | `premium:comparison-pack-01` | Motion Graphics | Implementado e validado: dois painéis responsivos, seis slots de texto, animação, receita IA e renderização QPainter |
| 14 | `flowchart-pack-01` | Motion Graphics | Planejado |
| 15 | `statistic-callout-pack-01` | Motion Graphics | Planejado |
| 16 | `cause-effect-pack-01` | Motion Graphics | Planejado |
| 17 | `film-grain-overlay-01` | Editorial Overlays | Planejado |
| 18 | `light-streak-overlay-01` | Editorial Overlays | Planejado |
| 19 | `scanline-overlay-01` | Editorial Overlays | Planejado |
| 20 | `vignette-overlay-01` | Editorial Overlays | Planejado |
| 21 | `lens-glow-overlay-01` | Editorial Overlays | Planejado |
| 22 | `lower-third-editorial-01` | Editorial Overlays | Planejado |
| 23 | `source-label-01` | Editorial Overlays | Planejado |
| 24 | `premium:title-reveal-01` | Kinetic Typography | Implementado e validado: wipe progressivo QPainter, subtítulo/identificação editáveis, ajuste para texto longo e layout retrato |
| 25 | `dramatic-quote-01` | Kinetic Typography | Planejado |
| 26 | `punch-word-01` | Kinetic Typography | Planejado |
| 27 | `side-caption-01` | Kinetic Typography | Planejado |
| 28 | `stacked-headline-01` | Kinetic Typography | Planejado |
| 29 | `number-emphasis-01` | Kinetic Typography | Planejado |
| 30 | `question-hook-01` | Kinetic Typography | Planejado |

Os manifestos versionados são embutidos no aplicativo e conferidos no início contra ID, tipo, árvore, tipos de camada e papéis de slot gerados pelo registro C++ em assets/premium/. A inicialização falha claramente se manifesto e construtor divergirem. Os 11 filtros temáticos existentes permanecem disponíveis. Os seis slots do Comparison Pack são somente textos; ele não anuncia slots de imagem. Receitas IA referenciam IDs reais por `kind: library`; a personalização dos slots ocorre depois de inserir o recurso no editor. Valores demonstrativos não são dados factuais. Slots de vídeo/ícone, pacotes externos e 27 recursos planejados continuam indisponíveis.

Arquivos de inspeção visual e benchmarks locais ficam em artifacts/premium-stage10/ (ignorado pelo Git).
