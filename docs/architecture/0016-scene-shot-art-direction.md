# ADR 0016: direção de arte por cena e shot

## Estado

Implementada para receitas v3 e projetos schema 11.

## Decisão

A receita aceita `artDirection` na raiz, em cada cena e em cada shot. Os objetos são parciais e se combinam na ordem global → cena → shot. Para cada campo, o valor local mais próximo substitui o valor herdado; campos não informados continuam herdados. `color`, `secondaryColor` e `fontSize` explícitos no elemento têm precedência sobre os defaults do estilo.

Na compilação, cada marcador de cena ou shot que contém override guarda o estilo efetivo resolvido. O projeto permanece editável e salva esses overrides no schema 11. Quando o inspetor cria um override local, ele copia o estilo efetivo do escopo pai; edições futuras do pai não alteram o snapshot local. Isso torna a aparência reproduzível depois da importação.

O compositor seleciona o estilo do shot ativo, depois o estilo da cena e por fim o estilo global. Essa resolução vale para o fundo da composição, fontes, cartões de legenda e destaque da palavra. Durante uma transição, cada camada de shot desenha com o próprio estilo; as legendas usam o estilo do shot entrante. A renderização do preview e do MP4 compartilha essa lógica.

## Consequências

- As receitas v1/v2 e as receitas v3 sem overrides locais preservam seu comportamento.
- Projetos schema 1–10 continuam carregando sem migração e sem mudanças visuais. Overrides locais exigem schema 11.
- O editor permite selecionar o escopo global, cena atual ou shot atual; cada edição passa pela validação de projeto e pelo histórico de undo.
- Cores e tamanho de fonte explícitos em objetos compilados permanecem preservados após mudanças de estilo. Fontes externas, presets nomeados e paletas pré-visualizáveis continuam pendentes.
- Um override de projeto é salvo como estilo efetivo completo. A edição local posterior não mantém vínculos vivos com os campos do pai.

## Verificação

Os testes cobrem a herança parcial no parser, as cores resolvidas nos elementos, a persistência de estilos de cena/shot e a diferença visual entre shots. O smoke de interface cobre a inicialização da janela; o smoke de produção valida guia/catálogo e mantém a comparação de preview/exportação.
