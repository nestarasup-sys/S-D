# ADR 0025 — cache e seek de vídeo

- Estado: Aceita
- Data: 2026-10-25

## Contexto

O compositor compartilhado atendia a prévia, still e exportação, mas buscava quadros antigos reiniciando a leitura desde o começo da fonte. Saltos muito distantes também podiam decodificar muitos quadros que não seriam exibidos.

## Decisão

- Manter um cache LRU de imagens de vídeo com limite agregado de 64 MiB por `CompositionPainter`.
- Identificar cada imagem por arquivo e revisão (tamanho e data de modificação), dimensões de saída, FPS, trim, velocidade, recorte e quadro local.
- Continuar decodificando sequencialmente em vizinhanças curtas. Para saltos acima de 24 quadros ou buscas para trás, iniciar FFmpeg com seek temporal próximo ao quadro solicitado.
- Calcular o tempo de seek a partir do primeiro quadro real com timestamp igual ou posterior ao trim da fonte. Essa correção impede deslocamento de um quadro quando o trim cai entre quadros da mídia.
- Preservar a composição e a sequência de filtros já compartilhadas por prévia e exportação. Não alterar o formato JSON nem o schema do projeto.
- Manter o áudio da prévia em `QMediaPlayer`; o controlador posiciona cada faixa a partir do quadro da timeline, do trim e da velocidade, deixando a busca do arquivo para Qt Multimedia.

## Consequências

- Repetir ou revisitar quadros dentro do limite do cache evita decodificação repetida. A memória usada para quadros de vídeo permanece limitada.
- Busca direta reduz o trabalho proporcional ao tamanho de vídeos longos. A busca continua dependendo de FFmpeg e da capacidade de seek do contêiner da mídia.
- Arquivos substituídos no mesmo caminho invalidam entradas antigas quando tamanho ou data de modificação mudam.
- Mídias com timestamps inconsistentes continuam sujeitas à interpretação do FFmpeg; o compositor e a exportação compartilham a mesma base de filtros.

## Verificação

- `scripts/build.ps1 -Test`: 4/4 testes passaram.
- `scripts/smoke-production.ps1`: passou com voz Microsoft Maria Desktop.
- Uma sequência de quadros solicitada fora de ordem registrou dois hits de cache, dois seeks diretos e três quadros decodificados. O quadro buscado aleatoriamente comparado ao quadro 55 do MP4 marcou 32,29 dB.
- A exportação do Director's Cut conservou 2700 quadros e marcou 42,46 dB entre quadros de prévia e MP4.
