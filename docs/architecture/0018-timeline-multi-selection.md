# ADR 0018 — seleção múltipla e movimento conjunto na timeline

## Estado

Implementada para camadas visuais do editor.

## Decisão

- A ponte do editor mantém uma lista de IDs selecionados e um ID principal. O inspetor continua operando sobre o ID principal para não aplicar alterações ambíguas em propriedades heterogêneas.
- Ctrl+clique adiciona ou remove um clipe sem iniciar o gesto de arraste. Clicar sem Ctrl em um clipe fora da seleção substitui a seleção; clicar em um clipe já selecionado preserva o conjunto para permitir arrastá-lo.
- O início do gesto captura os IDs, inícios de timeline e filhos de grupos selecionados. Os movimentos subsequentes usam essa origem fixa, aplicam um delta comum e deslocam também os keyframes.
- O delta é limitado pela interseção dos intervalos válidos dos clipes selecionados, incluindo limites da sequência e da cena de cada camada. Isso mantém o conjunto dentro de seus intervalos sem alterar os espaçamentos entre clipes.
- O histórico do editor agrupa o movimento em uma única ação de undo/redo. Os IDs e as linhas do modelo permanecem estáveis durante o gesto.
- A seleção múltipla é uma ferramenta de edição manual. Ela não altera o schema do projeto nem adiciona campos ao contrato JSON de produção.

## Consequências e limites

- A seleção da interface fica restrita às camadas da cena visível. O movimento respeita os limites próprios de cada clipe nessa cena.
- O inspetor não oferece edição em lote; ele permanece no item principal. Envelopes gráficos de áudio, seleção retangular e comandos de alinhamento continuam fora desta entrega.
- Ao selecionar um grupo, seus descendentes acompanham o grupo automaticamente. A seleção explícita de um filho junto ao grupo não duplica o deslocamento.

## Verificação

O teste do controlador verifica seleção aditiva, preservação de espaçamento, undo e redo do conjunto. A compilação também valida a interface QML e o binding da lista de IDs.
