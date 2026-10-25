# ADR 0009: shots independentes e Visual Beats

**Status:** aceito e implementado parcialmente na receita v3 e no projeto schema 4.

## Contexto

Uma cena narrativa precisava aceitar vários cortes sem duplicar nem interromper as falas. As receitas v1/v2 e o projeto schema 3 já trabalham com áudio medido, cues, composição compartilhada entre prévia/render e keyframes editáveis.

## Decisão

- A receita v3 substitui `elements`/`camera` de uma cena por `shots`; cada shot declara elementos próprios, câmera inicial e início temporal.
- Os inícios são convertidos em quadros a partir das amostras reais de voz (24 kHz, 800 amostras por quadro a 30 FPS). `cueProgress`/`progress` usa o primeiro e o último quadro audível da fala; `offsetSeconds` é a alternativa absoluta relativa ao cue.
- O próximo início encerra o shot atual. O último shot termina no fim resolvido da cena. A validação rejeita planos vazios, fora da cena, com lacunas ou sobreposição.
- A v3 mantém `transitionIn: "cut"` e aceita um objeto `transitionIn` para fade, dissolve, slide ou zoom, com duração em quadros; slide declara direção. O compositor sobrepõe a imagem final do shot anterior e a composição do shot entrante sem alongar o áudio nem o intervalo total da cena.
- Os marcadores de shot guardam tipo, direção e duração da transição em campos opcionais do schema 4. Projetos antigos continuam lendo `transitionIn: "cut"` e assumem duração zero.
- Visual Beats aceitos são `show`, `hide`, `transform` e `camera`. Transformações e câmera são compiladas em keyframes comuns do projeto para compartilhar avaliação na prévia e exportação. Os marcadores de beats permanecem serializados para a timeline e diagnóstico.
- Projetos schema 4 guardam marcadores de shots/beats, a associação `Node.shotId` e a associação `CameraKeyframe.shotId`. Projetos v1/v2/v3 continuam legíveis; composições antigas mantêm o caminho de compilação anterior.
- Arrastar uma alça de corte ajusta as durações adjacentes, desloca os elementos, beats e keyframes de câmera do shot seguinte e agrupa o gesto como uma ação de undo. O corte não atravessa elementos, beats ou keyframes do plano anterior.
- Os controles de câmera existentes no inspetor passam a gravar keyframes com o ID do shot ativo; esses estados acompanham o plano ao mover o corte.
- A receita v3 pode declarar `camera.focus` com um ID de elemento do mesmo shot e uma margem. Na compilação, os limites locais passam pelas transformações do elemento e de seus grupos ancestrais; o centro e o zoom resultantes definem a câmera no primeiro quadro do shot, mantendo sua rotação.
- `camera.focus.follow: true` recalcula os limites em cada quadro e emite keyframes de câmera comuns e editáveis. O modo de acompanhamento não aceita Visual Beats de câmera no mesmo shot para evitar duas fontes concorrentes de direção.

## Consequências

- Uma cena pode conter vários planos visuais com uma única faixa de narração e legenda.
- A duração dos planos se adapta à fala realmente sintetizada, inclusive em receitas de duração fixa já suportadas; a transição mistura imagens sem estender essa faixa de áudio.
- O editor pode modificar o corte sem destruir IDs; a movimentação do plano seguinte preserva seus offsets temporais.
- Elementos da v3 ainda não viram faixas de áudio ou vídeo, e não há efeitos por beat ou eventos por palavra.
- O acompanhamento contínuo é convertido em keyframes para compatibilidade com avaliação comum da prévia/exportação e edição manual; pode aumentar o número de keyframes em takes longos. Presets reutilizáveis e trajetórias Bézier ainda estão pendentes.
- Durante o overlap visual, a imagem de saída é congelada no último quadro do shot anterior. Camadas de vídeo com movimento contínuo poderão exigir que a mídia de saída continue tocando durante o overlap.

## Verificação

- Testes unitários verificam parsing, resolução de tempo, cobertura de shots, keyframes de beats, transições, persistência no schema 4 e undo de corte.
- `scripts/smoke-production.ps1` sintetiza com voz pt-BR, importa o exemplo v3, renderiza MP4 e compara quadros da prévia nos limites de shots/beats com a exportação.
