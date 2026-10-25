# ADR 0012: Sincronizar shots e Visual Beats por palavra

## Status

Implementada parcialmente. A sincronização está disponível para início de shot e evento de Visual Beat em receitas v3.

## Contexto

`cueProgress` posiciona um evento proporcionalmente à duração da fala, mas não consegue garantir que uma palavra específica já foi pronunciada. A receita precisa usar timestamps produzidos pela mesma voz local que gera o áudio final.

## Decisão

- O script Windows `System.Speech` recolhe eventos `SpeakProgress` para cada palavra durante a síntese pt-BR.
- O manifesto de voz guarda texto, posição/caracteres e posição de áudio em amostras de 24 kHz.
- O worker calibra os eventos para a duração real do WAV, preserva sua ordem e aplica o mesmo fator quando a fala é acelerada no modo de duração fixa.
- Em receita v3, `{"cue":0,"wordIndex":3}` referencia a quarta palavra da fala (`cue` começa em zero). A receita usa essa referência no `start` do shot ou no `at` do Visual Beat.
- O compilador rejeita índices ausentes e transforma o evento em quadros/markers editáveis, sem guardar dependência do sintetizador no projeto.

## Consequências

- A produção local pode acionar um corte ou evento visual em um limite de palavra real da voz selecionada.
- A IA precisa contar o índice de palavras dentro do texto exato e não deve alterar a narração depois de escolher os índices.
- A implementação inicial é Windows/SAPI pt-BR e limita sincronização por palavra a shots e Visual Beats. Elementos e keyframes, outros idiomas e alinhamento fonético ficam para etapas futuras.
