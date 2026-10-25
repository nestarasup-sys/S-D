# ADR 0028 — ajuste do overlap de shots na timeline

- Estado: Aceita
- Data: 2026-10-25

## Contexto

Shots de uma cena formam uma sequência contínua. Mover um corte compartilhado já ajustava a duração dos shots vizinhos e deslocava os elementos, beats e keyframes do plano seguinte. Transições animadas já tinham duração em quadros, mas seu overlap só podia ser alterado no inspetor.

## Decisão

- Manter um único limite compartilhado entre shots consecutivos; arrastá-lo muda a duração dos dois planos sem criar lacunas ou tempo adicional na cena.
- Exibir a duração da transição na faixa âmbar do shot de entrada e permitir ajustar seu fim com uma alça dedicada.
- Limitar a transição a 2–300 quadros e à duração disponível do shot de entrada, respeitando a validação do projeto.
- Agrupar cada arraste em uma ação de histórico para que undo/refazer restaure a transição inteira.
- Preservar o formato e schema de projeto existentes; transições continuam sendo sobreposições visuais e não estendem a narração.

## Consequências

- A duração de cada shot pode ser ajustada diretamente movendo o corte compartilhado. Elementos, beats, keyframes e áudio sonoro ligado ao shot acompanham a mudança.
- O overlap pode ser ajustado visualmente na timeline e numericamente pelo inspetor.
- Transições continuam sem acrescentar frames à sequência e precisam caber no shot de entrada.

## Verificação

- Teste do controlador altera a duração da transição dentro do limite mínimo e confirma undo/redo como gesto único.
- `scripts/smoke-workspace.ps1` confirma a abertura do editor sem erros QML; a captura do projeto v3 confirma a faixa de transição e a alça na timeline.
