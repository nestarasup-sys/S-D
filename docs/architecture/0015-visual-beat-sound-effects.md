# ADR 0015: Efeitos sonoros disparados por Visual Beats

## Status

Implementada em receitas v3 e projetos schema 9.

## Decisão

- Um Visual Beat com `action: "sound"` referencia um arquivo local por `source` e aceita `gain` entre 0 e 4 (padrão 1). Ele não usa `target`, duração de animação nem propriedades visuais.
- O compilador resolve `at` com o mesmo mecanismo de cue, progresso, deslocamento ou alinhamento por palavra usado pelos outros beats. No quadro resolvido, cria uma pista de áudio com ID `sfx-<beat-id>` e mantém o marcador ligado a ela por `targetId`.
- A pista começa no quadro do beat e termina no fim da cena. O player e o render usam o pipeline normal de pistas, então o áudio toca uma vez conforme o tamanho da fonte e não exige um segundo mixer.
- O limite de 32 pistas da receita inclui música/áudio declarado pela cena e efeitos gerados por beats. Caminhos relativos são resolvidos ao lado do arquivo `.production.json`; URLs e arquivos ausentes são rejeitados.
- Projetos schema 9 persistem o marcador e a pista vinculada. A validação exige que o início da pista corresponda ao quadro do beat e rejeita referências duplicadas ou ausentes.
- Reposicionar a pista move o marcador; arrastar o corte do shot move o beat e preserva o fim da faixa; excluir a pista remove também seu marcador. Cada ação continua sendo editável e undoable pelo editor.

## Consequências

- Preview e exportação compartilham o mesmo processamento de áudio já utilizado pelas pistas normais.
- Projetos schema 1–8 e receitas v1–v3 sem a nova ação continuam aceitos.
- A fonte continua externa ao JSON. Para compartilhar uma receita, inclua o arquivo de áudio ou use um caminho que exista no computador de destino.
- O projeto pode conter silêncio após o fim natural do arquivo até o fim da faixa/cena; essa área permanece útil para mover e ajustar a pista na timeline.
