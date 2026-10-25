# ADR 0026 — trajetórias reutilizáveis de câmera

- Estado: Aceita
- Data: 2026-10-25

## Contexto

Receitas v3 permitem definir o enquadramento inicial por shot e adicionar alterações de câmera como Visual Beats. Repetir um mesmo movimento em vários planos exigia duplicar estados e tempos, o que facilitava divergências entre cortes.

## Decisão

- Declarar trajetórias nomeadas no objeto opcional de raiz `cameraPresets`.
- Referenciar a trajetória pelo nome em `scenes[].shots[].cameraPreset`.
- Cada estado tem tempo relativo ao começo do shot e uma posição completa (`x`, `y`, `zoom`, `rotation`), com curva opcional para a interpolação até o próximo estado.
- O compilador expande o preset em keyframes de câmera do projeto, associados ao shot. A prévia, timeline, undo, persistência e exportação reutilizam o formato de câmera já existente; nenhum schema novo de projeto é necessário.
- Limitar a 32 definições, 512 estados declarados e 4096 estados expandidos por produção. A trajetória começa no offset zero, avança pelo menos um quadro e deve terminar dentro do shot.
- Rejeitar a combinação de preset com `focus.follow` e Visual Beats de câmera no mesmo shot para manter uma só fonte de movimento no intervalo.

## Consequências

- A IA pode declarar uma trajetória uma vez e reutilizá-la entre planos; os keyframes resultantes continuam editáveis.
- Alterar a declaração afeta todos os shots que referenciam o preset durante uma nova importação. Depois de compilado, o projeto guarda keyframes independentes.
- Presets são definidos e reaplicados na receita JSON. Ainda não há interface na biblioteca para salvar, renomear ou organizar presets de câmera.
- Cortes orientados pelo conteúdo e sobreposição/duração avançada da sequência permanecem fora deste recurso.

## Verificação

- Testes automatizados verificam parse dos nomes/estados, rejeição de referência ausente, expansão do mesmo preset em dois shots e keyframes editáveis.
- `scripts/smoke-production.ps1` compila e renderiza o exemplo v3 com o preset, abre o projeto no editor e compara preview/export. A comparação do exemplo v3 marcou 44,32 dB.
