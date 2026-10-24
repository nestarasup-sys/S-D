# ADR 0009: shots independentes e Visual Beats

**Status:** aceito e implementado parcialmente na receita v3 e no projeto schema 4.

## Contexto

Uma cena narrativa precisava aceitar vários cortes sem duplicar nem interromper as falas. As receitas v1/v2 e o projeto schema 3 já trabalham com áudio medido, cues, composição compartilhada entre prévia/render e keyframes editáveis.

## Decisão

- A receita v3 substitui `elements`/`camera` de uma cena por `shots`; cada shot declara elementos próprios, câmera inicial e início temporal.
- Os inícios são convertidos em quadros a partir das amostras reais de voz (24 kHz, 800 amostras por quadro a 30 FPS). `cueProgress`/`progress` usa o primeiro e o último quadro audível da fala; `offsetSeconds` é a alternativa absoluta relativa ao cue.
- O próximo início encerra o shot atual. O último shot termina no fim resolvido da cena. A validação rejeita planos vazios, fora da cena, com lacunas ou sobreposição.
- A v3 aceita `transitionIn: "cut"`. Cada shot define uma composição e estado inicial de câmera independentes; não há sobreposição temporal de shots nesta versão.
- Visual Beats aceitos são `show`, `hide`, `transform` e `camera`. Transformações e câmera são compiladas em keyframes comuns do projeto para compartilhar avaliação na prévia e exportação. Os marcadores de beats permanecem serializados para a timeline e diagnóstico.
- Projetos schema 4 guardam marcadores de shots/beats, a associação `Node.shotId` e a associação `CameraKeyframe.shotId`. Projetos v1/v2/v3 continuam legíveis; composições antigas mantêm o caminho de compilação anterior.
- Arrastar uma alça de corte ajusta as durações adjacentes, desloca os elementos, beats e keyframes de câmera do shot seguinte e agrupa o gesto como uma ação de undo. O corte não atravessa elementos, beats ou keyframes do plano anterior.

## Consequências

- Uma cena pode conter vários planos visuais com uma única faixa de narração e legenda.
- A duração dos planos se adapta à fala realmente sintetizada, inclusive em receitas de duração fixa já suportadas.
- O editor pode modificar o corte sem destruir IDs; a movimentação do plano seguinte preserva seus offsets temporais.
- Elementos da v3 ainda não viram faixas de áudio ou vídeo, e não há efeitos por beat, transições com sobreposição, eventos por palavra ou câmera focada por ID.
- Os controles numéricos da câmera de shot no inspetor ainda estão pendentes; a câmera pode ser ajustada no JSON e seus keyframes/estado podem ser movidos junto ao corte.

## Verificação

- Testes unitários verificam parsing, resolução de tempo, cobertura de shots, keyframes de beats, schema 4 e undo de corte.
- `scripts/smoke-production.ps1` sintetiza com voz pt-BR, importa o exemplo v3, renderiza MP4 e compara quadros da prévia nos limites de shots/beats com a exportação.
