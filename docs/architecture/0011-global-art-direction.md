# ADR 0011: Direção de arte global da produção

## Status

Aceita e implementada parcialmente em receitas v3 e projetos schema 4.

## Contexto

Uma direção visual coerente precisa ser declarada uma vez e aplicada tanto à prévia quanto à exportação. Valores enviados explicitamente para um elemento pela IA também precisam continuar precisos.

## Decisão

- `ProductionRecipe` e `Project` carregam um `ArtDirection` com paleta, fonte, tamanhos tipográficos, posição das legendas e margem segura.
- A receita v3 aceita o objeto opcional `artDirection`. Projetos o persistem sob a raiz em `artDirection`; versões de projeto anteriores carregam os defaults.
- Na importação, a direção global preenche cor, cor secundária e tamanho de texto ausentes. Valores explícitos no elemento vencem os defaults globais.
- O compilador aplica o tema a fundo, cabeçalhos produzidos automaticamente e legendas.
- O `CompositionPainter` consome o mesmo tema na prévia e na renderização.
- Campos são validados e limitados antes da compilação; nomes de fonte referem-se a fontes já instaladas no sistema.

## Consequências

- A IA pode especificar uma paleta uma vez, sem repetir cores em cada objeto.
- Projetos antigos continuam abrindo com os defaults visuais anteriores.
- Ainda não há controles do tema na interface, overrides por cena/shot, carregamento de fontes próprias nem presets de tema.
