# ADR 0020 — zoom temporal e navegação da timeline

## Estado

Implementada na interface QML do editor.

## Decisão

- A timeline mantém um fator de zoom temporal de 1× a 8× e um deslocamento horizontal normalizado sobre o intervalo que não está visível.
- O zoom pode ser alterado no controle da interface ou com Ctrl+roda sobre a régua. O ponto do ponteiro permanece ancorado no frame equivalente durante a aproximação ou afastamento.
- Shift+roda e a barra horizontal movem o intervalo visível. A régua converte cliques/arrastes em frames dentro desse intervalo.
- Clips, shots, transições, Visual Beats, playhead e envelopes consultam as mesmas funções de conversão frame↔pixel do painel. Arrastes de clipe e de corte aplicam deltas relativos à quantidade de frames visível.
- Zoom e deslocamento pertencem à visualização da timeline e não alteram o projeto ou seu schema.

## Consequências e limites

- Clipes curtos podem ocupar mais pixels conforme o zoom, permitindo reposicionar o tempo com mais precisão sem alterar a sequência.
- A precisão final continua quantizada a frames; alinhamento magnético/snapping, entrada numérica de frame e zoom acima de 8× permanecem pendentes.
- O intervalo volta a ser representado proporcionalmente se a duração do projeto mudar; o estado da visualização não é persistido no arquivo.

## Verificação

`scripts/build.ps1 -Test` passou (4/4) e `scripts/smoke-workspace.ps1` abriu o editor sem erros QML. A captura verifica os controles e a régua; a suíte ainda não simula eventos de roda/slider.
