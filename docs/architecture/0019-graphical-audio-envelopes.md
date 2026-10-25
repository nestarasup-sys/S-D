# ADR 0019 — edição gráfica de envelopes de áudio

## Estado

Implementada no editor para automação de ganho das pistas de áudio.

## Decisão

- Cada pista mostra uma amostra da curva de automação sobre sua posição e duração na timeline. A curva representa o multiplicador `gainAutomation.value` de 0 a 4; o ganho base da pista permanece no controle existente.
- Clicar na faixa cria ou substitui um ponto no frame correspondente. Arrastar um ponto altera seu frame local e multiplicador. Um ponto existente mantém seu easing durante o arraste; um ponto novo usa o easing escolhido na linha de controles.
- Prévia e exportação continuam avaliando o mesmo vetor de pontos/easing persistido em `AudioTrack`. A interação gráfica chama o controlador C++ e não altera dados diretamente no QML.
- Um gesto usa o histórico existente do editor, de modo que um arraste inteiro é uma única ação de undo/redo. Limites do schema continuam valendo: 128 pontos por faixa, frames inteiros em ordem crescente e easing suportado.
- A receita v3 permanece inalterada. A automação continua podendo ser declarada em `gainAutomation`; o desenho no editor é um recurso manual após importar ou adicionar uma pista.

## Consequências e limites

- A pessoa pode criar e ajustar o envelope visualmente sem navegar o playhead para cada ponto. Os controles existentes continuam disponíveis para definir valores exatos e limpar a automação.
- O gráfico usa amostragem visual da curva atual; os pontos persistidos permanecem a fonte de verdade. Isso não altera a precisão da avaliação por frame usada no áudio.
- A interface não altera automaticamente o ganho base e ainda não fornece trim de origem, loop de faixa ou waveform individual da mídia.

## Verificação

O teste do controlador verifica a lista de amostras, ordenação e persistência dos valores/easing, além de undo unitário do gesto. `scripts/build.ps1 -Test`, `scripts/smoke-workspace.ps1` e `scripts/smoke-production.ps1` verificam compilação, abertura QML e compatibilidade com mix/exportação.
