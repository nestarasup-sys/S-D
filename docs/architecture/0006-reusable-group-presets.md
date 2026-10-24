# ADR 0006 — presets de grupos reutilizáveis

Data: 24/10/2026. Estado: implementado e verificado localmente.

## Motivação

A biblioteca aceitava um único nó sem keyframes, apesar de projetos schema 3 já suportarem grupos hierárquicos e animações. A IA externa não conseguia reutilizar uma composição inteira salva pelo usuário.

## Decisão

- O comando **Salvar seleção do editor** admite um nó raiz e todos os descendentes, coletados pelos vínculos `parentId`, independentemente da ordem das linhas.
- O preset é armazenado no formato de projeto schema 3, com início normalizado para frame zero, duração da raiz, hierarquia e keyframes relativos. Imagens e SVGs são copiados por hash para a biblioteca; a categoria e o tombstone continuam gerenciados pelo catálogo.
- A reinserção no editor gera IDs novos para cada nó, remapeia os pais, preserva a animação e faz uma única transação de histórico. Se a composição não couber no intervalo restante da cena, falha sem alterar o documento.
- A receita `studio-dark-production` v2 permite `kind: "library"` apontando para um preset composto. O parser expande filhos em memória e os inclui no limite de 200 nós/8 níveis. O compilador resolve o tempo de cada parte e keyframe relativo ao cue medido da voz. Chaves que excedem o trecho resultam em erro explícito.
- Presets legados de nó único sem keyframes continuam sendo estendidos até o final da cena, como antes.

## Limitações

O manifesto pessoal depende da instalação e os assets não são empacotados no JSON. Presets de composição não são instâncias vinculadas nem aceitam retiming automático; não há multi seleção, curvas Bézier personalizadas, vídeo como camada nem componentes com parâmetros editáveis em lote. A animação aceita as curvas nomeadas `linear`, `ease-in`, `ease-out` e `ease-in-out`.

## Verificação

Foram acrescentados testes automatizados para salvamento, reinserção, undo, IDs, hierarquia, keyframes e compilação por JSON. Em 24/10/2026, `scripts/build.ps1 -Test` passou (4/4), assim como `scripts/smoke.ps1`, `scripts/smoke-workspace.ps1`, `scripts/smoke-production.ps1` e `scripts/update-ai-guide.ps1`. O smoke de produção usa a voz SAPI instalada `Microsoft Maria Desktop` e compara quadros do MP4 renderizado com a composição.
