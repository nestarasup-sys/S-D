# ADR 0021 — edição manual dos tempos de palavra

Data: 25/10/2026. Estado: implementado e verificado em Windows.

## Contexto

Receitas v3 podem produzir legendas com limites de palavra medidos pela voz pt-BR local. Até agora, depois da importação, esses timestamps só podiam ser alterados indiretamente ao mudar a fala e gerar outra produção.

## Decisão

- Expor no inspetor da legenda os intervalos absolutos início/fim de cada palavra alinhada, em quadros do projeto; na timeline, exibir uma faixa segmentada com alças para os limites.
- Selecionar uma camada revela sua faixa na timeline. Ao incluir várias pistas de áudio, a lista dessas pistas rola separadamente e conserva altura para as camadas visuais.
- Permitir entrada numérica e incremento/decremento de um quadro, preservando pelo menos um quadro por palavra.
- Tratar a borda entre palavras como um único limite: editar o fim de uma palavra também ajusta o início da seguinte, e vice-versa.
- Arrastar uma alça converte sua posição horizontal para o quadro mais próximo e agrupa as atualizações de movimento em uma única ação de histórico.
- Rejeitar alterações que saiam da duração da legenda ou eliminem um intervalo válido. Uma borda editada é uma ação normal do histórico e pode ser desfeita/refeita.
- Manter a representação existente de `captionWords` e o schema 7; receitas não recebem novos campos, pois esses ajustes acontecem depois da compilação.

## Verificação

Os testes do controlador cobrem leitura dos intervalos, edição de limite compartilhado, validação, agrupamento de quadros sucessivos em uma ação undo/redo e compatibilidade do modelo existente. `scripts/build.ps1 -Test` passou (4/4), `scripts/smoke-workspace.ps1` passou e `scripts/smoke-production.ps1` abriu uma legenda ativa com os controles no inspetor e sua faixa revelada na timeline, usando Microsoft Maria Desktop.
