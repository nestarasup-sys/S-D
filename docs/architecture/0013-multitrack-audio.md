# ADR 0013: Pistas de áudio independentes e ducking

## Status

Implementada parcialmente em projeto schema 4, receita v3, editor e exportador.

## Decisão

- Cada `AudioTrack` guarda identificador, nome, caminho local, início, duração, ganho, mute e configuração `duckUnderVoice`/`duckAmount`.
- Projetos persistem até 32 pistas; receitas v3 podem declarar faixas por cena com um início sincronizado à fala.
- A prévia cria um player Qt por pista e inicia/para cada uma com o relógio da sequência.
- A exportação faz trim, ganho e atraso por pista em um `filter_complex` FFmpeg, mistura tudo com limite de pico e codifica para AAC.
- `compileProduction` registra separadamente o WAV final de áudio base e o stem isolado da voz. O sidechain comprime o clip de música/efeito durante a fala e recupera o ganho no silêncio.
- Projetos manuais sem stem dedicado podem apontar `audioSource` como sidechain. Ducking sem qualquer referência de voz é inválido.

## Limitações

- O ducking é aplicado na exportação; a prévia toca as fontes em paralelo sem compressão sidechain.
- Fade in/out, automação de ganho em keyframes, modo de looping e edição de envelope continuam pendentes.
- Cada faixa começa no início da própria fonte; não há trim-offset de entrada nem waveform individual ainda.
- Fontes são locais; não há busca ou download remoto de música.
