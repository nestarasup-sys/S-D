# Storyboard — demonstração premium do Studio Dark

Receita de produção: `examples/premium-cinematic-demo-v3.production.json`.

O projeto e os arquivos de inspeção/exportação ficam em `artifacts/premium-demo-stage10-20261026-205100/` (pasta local ignorada pelo Git). A renderização utilizou Microsoft Daniel pt-BR e alinhamento de palavras do Windows Speech; os tempos abaixo foram calculados pela produção real, não estimados pelo texto.

| Shot | Intervalo de referência | Recurso e ação | Direção visual |
| --- | --- | --- | --- |
| Abertura — Title Reveal | 00:00.00–00:05.03 | Título e linha editorial entram com wipe progressivo QPainter. | Fundo azul petróleo, acento verde e título em duas linhas. |
| Objeto — Smartphone premium | 00:05.03–00:15.23 | Smartphone em camadas com tela substituída por PNG, texto editorial e notificação editável; câmera aproxima durante a fala. | A composição evidencia moldura, reflexo, tela e mensagem como camadas distintas. |
| Comparação — dois cenários | 00:15.23–00:23.90 | Dois painéis com seis textos editados para conteúdo demonstrativo; dissolve de entrada. | Comparação qualitativa, sem números apresentados como fatos. |
| Encerramento — ideia em foco | 00:23.90–00:27.17 | Segunda instância do Title Reveal, dissolução e beat de câmera fecham a composição. | Reapresenta o título em um shot próprio; entrada/saída ajustadas ao fim real da fala. |

## Personalizações feitas no projeto editável

- Título: “IDEIAS QUE / MOVEM HISTÓRIAS”; identificação e subtítulo personalizados.
- Tela do smartphone: PNG original criado para a demonstração, salvo em `media/premium-screen-slot.png`.
- Textos da tela e da notificação substituídos pelos slots do smartphone.
- Títulos, valores e descrições dos dois painéis substituídos pelos seis slots do Comparison Pack. Os valores são demonstrativos e não são alegações jornalísticas.
- A imagem de tela e as alterações foram verificadas no projeto gerado e novamente lidas pelo renderizador.

## Quadros

Os PNGs representativos e `studio-dark-premium-demo.mp4` são exportados pelo renderizador oficial para `artifacts/premium-demo-stage10-20261026-205100/`. O MP4 usa H.264/AAC, 1280×720, 30 FPS, 815 quadros e dura 27,17 s. A validação comparou quatro quadros contra a referência QPainter, com PSNR entre 38,58 e 41,39 dB e MAE RGB entre 1,42 e 1,56. A prévia e o MP4 usam `CompositionPainter`/QPainter; a demonstração não depende do protótipo GPU.

## Limites

Os slots personalizados não são campos da receita JSON: a receita seleciona IDs reais; o projeto gerado recebe as alterações editáveis. O projeto referencia a imagem PNG local, portanto mantenha a pasta `media` junto da demonstração na instalação atual. O projeto não é um pacote portátil para outro computador.
