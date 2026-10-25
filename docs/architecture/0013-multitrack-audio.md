# ADR 0013: Pistas de áudio independentes e ducking

## Status

Implementada parcialmente em projetos schema 4–10, receita v3, editor e exportador. Efeitos disparados por Visual Beat são especificados na [ADR 0015](0015-visual-beat-sound-effects.md).

## Decisão

- Cada `AudioTrack` guarda identificador, nome, caminho local, início, duração, ganho, mute e configuração `duckUnderVoice`/`duckAmount`.
- `fadeInFrames` e `fadeOutFrames` opcionais aplicam rampas lineares de volume nos limites da faixa. Projetos com fades usam schema 6.
- `gainAutomation` guarda até 128 pontos locais à faixa com multiplicador de ganho 0–4 e curva `linear`, `ease-in`, `ease-out` ou `ease-in-out`; a validação exige quadros crescentes. Projetos com pontos usam schema 8.
- Projetos persistem até 32 pistas; receitas v3 podem declarar faixas por cena com um início sincronizado à fala.
- A prévia cria um player Qt por pista, inicia/para cada uma com o relógio da sequência e ajusta fades/ducking em tempo de reprodução. Com WAV PCM mono a 24 kHz, o exportador calcula a mesma envoltória RMS suavizada por quadro e aplica o arquivo de envelope à pista.
- A prévia atualiza o volume conforme automação, fades e ducking a cada quadro; a exportação usa expressão de ganho por tempo, filtros `afade` e `amultiply` do FFmpeg antes da mixagem.
- A exportação faz trim, ganho e atraso por pista em um `filter_complex` FFmpeg, mistura tudo com limite de pico e codifica para AAC.
- `compileProduction` registra separadamente o WAV final de áudio base e o stem isolado da voz. A envoltória suavizada mantém o mesmo ganho ao terminar a fala e evita que a faixa seja truncada quando o sidechain termina.
- Projetos manuais sem stem dedicado podem apontar `audioSource` como sidechain. Ducking sem qualquer referência de voz é inválido.

## Limitações

- Referências que não sejam WAV PCM mono a 24 kHz usam o sidechain nativo do FFmpeg na exportação; a prévia só calcula RMS em WAV compatível. Converta a voz para WAV PCM mono a 24 kHz para obter a mesma envoltória nos dois caminhos.
- Modo de looping e desenho gráfico livre dos envelopes continuam pendentes; os pontos são editados no playhead e ajustados pela lista da pista.
- Cada faixa começa no início da própria fonte; não há trim-offset de entrada nem waveform individual ainda.
- Fontes são locais; não há busca ou download remoto de música.
