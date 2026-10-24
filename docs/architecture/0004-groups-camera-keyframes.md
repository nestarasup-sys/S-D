# ADR 0004 — grupos, keyframes e câmera de composição

Data: 24/10/2026. Estado: adotada para a fundação da composição livre.

## Contexto

A próxima direção do produto pede que a IA combine elementos e conduza o olhar em um espaço contínuo. A composição anterior só possuía uma lista plana de objetos e animações de entrada/saída. Antes de acrescentar vídeos e documentos, o projeto precisa expressar hierarquia, movimento por tempo e enquadramento de forma persistente.

## Decisão

- O formato `studio-dark` versão 3 adiciona grupos, referências `parentId`, keyframes de transformação por nó e keyframes de câmera 2D.
- As posições dos filhos são locais ao grupo. Grupos podem conter grupos; ciclos e referências a pais que não sejam grupos são recusados.
- Keyframes guardam frame, posição, rotação, escala, opacidade e curva de interpolação até aquela chave. A avaliação aceita `linear`, `ease-in`, `ease-out` e `ease-in-out` (padrão linear em projetos antigos), mantendo o primeiro/último estado fora do trecho interpolado.
- A câmera guarda centro, zoom e rotação por frame. Sem keyframes, o enquadramento permanece equivalente ao quadro atual.
- Prévia e render worker chamam o mesmo avaliador e compositor C++ para cada instante.
- Projetos v1/v2 continuam legíveis e sem alteração visual. O arquivo v2 mantém sua versão quando salvo; receitas antigas continuam produzindo projetos v3 novos. Uma migração explícita de projetos existentes só será adicionada junto com os controles de composição da próxima etapa.

## Limites desta primeira entrega

O editor permite envolver um elemento por vez em um grupo, gravar/editar keyframes no playhead e definir keyframes de câmera pelo inspetor. Filhos são identificados na lista; arrastar o grupo na timeline desloca a hierarquia e as chaves no tempo; excluir um grupo exclui toda sua árvore. Ainda não há multi seleção, editor de curvas Bézier personalizadas, âncoras visuais ou alvos semânticos. Presets completos de composição foram implementados na ADR 0006. A atualização do contrato da receita para IA não faz parte deste marco; receitas v1 continuam compilando seus objetos planos no mesmo projeto editável.

O renderer permanece baseado em QPainter. Esta entrega não conclui o benchmark entre QPainter, Qt Quick Scene Graph e RenderControl proposto em `FUTURO-DO-APP.md`. O compositor aplica hierarquia por quadro; desempenho deve ser medido antes de elevar limites ou adicionar efeitos custosos.

## Validação

O build C++ é a verificação desta etapa. A suíte existente deve ser rodada quando a validação de código for solicitada; os cenários de formato foram atualizados para refletir que a versão 3 já é válida. O smoke de interface/render e o benchmark comparativo serão registrados como etapas posteriores, antes de declarar paridade visual e desempenho aprovados.
