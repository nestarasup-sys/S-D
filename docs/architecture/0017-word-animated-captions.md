# ADR 0017 — Animação de legendas por palavra

## Estado

Implementada para receitas v3 e projetos schema 12.

## Decisão

Cada palavra de uma legenda pode receber uma entrada visual configurável a partir do timestamp real emitido pela voz local. A receita define um preset global `wordAnimation`; após a montagem, cada legenda guarda o preset e os parâmetros no próprio nó para continuar editável sem depender da receita original.

Os presets são `highlight` (compatibilidade e padrão), `none`, `pop`, `rise`, `fade` e `pop-rise`. A entrada usa easing cúbico de saída. Escala inicial, deslocamento vertical e duração são validados e limitados. Palavras sem timestamp confiável não recebem uma estimativa baseada no texto; presets animados exigem alinhamento real.

O inspetor edita o preset e os parâmetros da legenda selecionada. Prévia e render usam o mesmo compositor por quadro. Os campos só são persistidos quando não estão no padrão, sob `nodes[].wordAnimation` em projetos schema 12; projetos anteriores mantêm o destaque estático já existente.

## Consequências

- A receita IA pode especificar entrada por palavra sem gerar keyframes duplicados para cada palavra.
- O projeto permanece editável, e o controle de uma legenda não altera as demais.
- A saída depende de a voz pt-BR local fornecer eventos reais de alinhamento.
- O guia descreve os nomes, defaults, limites e presets aceitos; não há animação de palavra no SRT.
