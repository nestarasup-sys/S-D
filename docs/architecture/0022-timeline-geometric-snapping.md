# ADR 0022 — snapping geométrico de clipes na timeline

Data: 25/10/2026. Estado: implementado e verificado em Windows.

## Contexto

O editor já permite arrastar uma ou mais camadas visuais e seus grupos/keyframes. Sem referências de alinhamento, colocar bordas de clipes em shots, beats ou áudio exige precisão manual por frame, especialmente quando o usuário aproxima ou afasta a timeline.

## Decisão

- Adicionar um controle Snap ligado/desligado na timeline; ligado por padrão para cada painel aberto. Desligado, o movimento preserva o comportamento livre.
- Durante o arraste, considerar início e fim do conjunto selecionado. Usar como referências o playhead, limites de cenas e shots, Visual Beats, bordas de camadas não selecionadas e início/fim das pistas de áudio.
- Derivar a tolerância de oito pixels visuais para frames, conforme o zoom atual. Mostrar uma linha-guia âmbar somente enquanto houver encaixe.
- Respeitar os limites existentes de sequência e cena. Grupos, keyframes e seleção múltipla continuam se movendo juntos e em um único gesto undo/redo.
- Adicionar uma alça de arraste no início do envelope das pistas de áudio. Ela move o início preservando a duração e sincroniza marcadores de Visual Beat `sound` ligados à pista; os demais controles do envelope continuam acessíveis.
- Adicionar uma alça de trim no fim do envelope de áudio. Ela altera a duração por frame, sem mover o início, e usa os mesmos limites de sequência e snap geométrico.
- Manter a função na camada de edição do app; nenhuma propriedade de receita JSON ou mudança de schema é necessária.

## Verificação

`scripts/build.ps1 -Test` passou (4/4); os testes do controlador confirmam encaixe de movimento e trim visual/áudio ao limite de shot dentro da tolerância, preservação do movimento quando a diferença é maior e sincronização/undo conjunto do beat sonoro vinculado. `scripts/smoke-workspace.ps1` e `scripts/smoke-production.ps1` passaram; a compilação gerou o QML e o smoke abriu projetos no editor. O guia da IA foi regenerado e esclarece que snapping é edição manual, sem campo correspondente no JSON.
