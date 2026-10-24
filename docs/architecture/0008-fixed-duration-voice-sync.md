# ADR 0008 — duração-alvo e sincronização proporcional à voz

Data: 24/10/2026. Estado: implementado no código da master; compilação e smoke Windows ainda pendentes.

## Motivação

Antes, a duração emergia de WAVs locais e pausas fixas. Uma IA não podia impor 2700 frames para 90 segundos ou ancorar movimentos na porcentagem de uma fala ajustada.

## Contrato

- Receita v2: `timingMode: natural` por padrão. `fit-to-duration` exige `targetDurationSeconds` global de 1 a 600 s; `preserveVoicePitch: true` é obrigatório se declarado.
- Cenas aceitam `targetDurationSeconds` opcional. Narrações aceitam string antiga ou `{text,targetDurationSeconds}`. Esse alvo de fala é uma **janela de voz e pausa**.
- Duracões são arredondadas a frames de 30 FPS; o somatório das cenas alocadas bate exatamente com o alvo.
- `cueProgress: 0..1` substitui `offsetSeconds` em elemento, keyframe ou câmera, ancorado no início e no último frame da **fala audível**, não na pausa. As duas referências temporais são mutuamente exclusivas em um mesmo objeto.
- O modo `natural` preserva seu planejamento anterior quando não há propriedades de duração-alvo.

## Produção

1. Gerar WAVs com voz local, PCM mono, 24 kHz; medir as amostras de cada trecho.
2. Planejar as janelas exatas de cena e fala em frames. Respeitar orçamentos explícitos, distribuir o tempo restante a partir do tamanho real das falas.
3. Limitar compressão por voz a 1,15×. Se o tempo planejado exigir mais, rejeitar com identificação da fala ou cena inviável.
4. No worker Windows, usar `STUDIO_FFMPEG` ou `ffmpeg` disponível no PATH e o filtro `atempo` apenas para trechos mais longos que seu orçamento; arredondar a amostra ao frame.
5. Montar pausas, marcadores, legendas e keyframes após a voz ser ajustada; mixer de trilha trabalha sobre a duração resultante.
6. Registrar modo, duração-alvo, total de frames e sincronização no relatório da produção, e etapas/erros no JSONL da importação IA.

## Limites e verificação

- Não há alinhamento por palavra, editor de velocidade de fala manual, alvo de câmera por elemento, ajuste temporal de keyframes preexistentes de presets, nem algoritmo de compressão sem FFmpeg.
- `atempo` tende a preservar o tom, mas qualidade depende do TTS e da fala. Ajustes acima de 15% são recusados; pausas longas merecem revisão editorial.
- A duração exata é propriedade do projeto em frames. MP4/AAC pode incluir priming e pequenas variações de metadados.
- O teste unitário simula o WAV já ajustado por tamanho de amostras; validação auditiva real exige Windows com voz pt-BR e FFmpeg disponível.
