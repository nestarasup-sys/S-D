# ADR 0013: Pistas de áudio independentes e ducking

## Status

Implementada parcialmente em projetos schema 4–6, receita v3, editor e exportador.

## Decisão

- Cada `AudioTrack` guarda identificador, nome, caminho local, início, duração, ganho, mute e configuração `duckUnderVoice`/`duckAmount`.
- `fadeInFrames` e `fadeOutFrames` opcionais aplicam rampas lineares de volume nos limites da faixa. Projetos com fades usam schema 6.
- Projetos persistem até 32 pistas; receitas v3 podem declarar faixas por cena com um início sincronizado à fala.
- A prévia cria um player Qt por pista, inicia/para cada uma com o relógio da sequência e ajusta fades/ducking em tempo de reprodução. O ducking usa uma envoltória RMS suavizada do WAV de voz; a exportação permanece baseada em sidechain FFmpeg.
- A prévia atualiza o volume conforme os fades por frame; a exportação usa filtros `afade` do FFmpeg antes do ganho, sidechain e mix.
- A exportação faz trim, ganho e atraso por pista em um `filter_complex` FFmpeg, mistura tudo com limite de pico e codifica para AAC.
- `compileProduction` registra separadamente o WAV final de áudio base e o stem isolado da voz. O sidechain comprime o clip de música/efeito durante a fala e recupera o ganho no silêncio.
- Projetos manuais sem stem dedicado podem apontar `audioSource` como sidechain. Ducking sem qualquer referência de voz é inválido.

## Limitações

- A prévia estima o sidechain pela energia RMS do WAV de referência e usa suavização; esse acompanhamento é aproximado e não equivale ao compressor FFmpeg da exportação. Sem um WAV legível, a prévia só aplica os fades.
- Automação com pontos de ganho/easing, modo de looping e edição gráfica de envelope continuam pendentes.
- Cada faixa começa no início da própria fonte; não há trim-offset de entrada nem waveform individual ainda.
- Fontes são locais; não há busca ou download remoto de música.
