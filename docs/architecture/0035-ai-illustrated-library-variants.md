# ADR 0035 — Variantes ilustradas da AI Asset Library v1

Data: 28/10/2026. Estado: implementado.

## Contexto

O pacote fornecido contém 48 ilustrações PNG transparentes associadas a IDs nativos existentes e 12 cenários rasterizados de 482–483×271–272 pixels. Substituir os desenhos existentes por PNGs alteraria projetos antigos e removeria recursos vetoriais/procedurais úteis, como poses e cores.

## Decisão

- Os 72 nativos originais permanecem registrados em `nativeLibrary()` e seus IDs e resultados visuais não mudam.
- As novas aparências são registradas separadamente como variantes estáticas com IDs estáveis `illustrated:<nativeId>`: por exemplo, `illustrated:car` deriva de `car`.
- Variantes são itens integrados `kind: native`, listados na biblioteca e no catálogo de capacidades. A receita pode usar `{"kind":"native","asset":"illustrated:car"}`; o mesmo ID também pode ser usado pelo item de biblioteca local.
- `LibraryItem` guarda o recurso embutido, dimensões lógicas, dimensões-fonte, nativo de fallback e marcação `previewOnly`. O catálogo foi semeado/migrado pelo `WorkspaceStore`, sem substituir ou reativar itens que o usuário moveu à lixeira.
- PNGs são empacotados como recursos Qt no compositor. O QPainter os compõe com o alpha e o filtro já usado para imagens; o movimento, a escala, a rotação, a opacidade, os keyframes, os grupos e a câmera continuam sendo os do nó comum. Se um recurso não puder ser lido, o compositor desenha o nativo procedural associado.
- Aparências PNG não aceitam cor/paleta, pose ou animação interna. A importação rejeita esses campos quando enviados para uma variante ilustrada. Propriedades gerais de transformação e animação do nó continuam válidas.
- As 12 cenas permanecem explicitamente marcadas como prévias de baixa resolução. A interface mostra a resolução real da imagem e o guia alerta contra seu uso como fundos finais Full HD. Os IDs procedurais simples continuam disponíveis.
- A lista de variantes derivada do pacote é conferida nos testes contra os 60 arquivos do manifesto. `nativeAssets` continua representando apenas os 72 vetores/procedurais; `illustratedAssets` descreve o catálogo rasterizado separadamente.

## Origem e limite

O pacote declara que a arte foi gerada para o usuário por IA. Isso registra a origem informada, mas não concede direitos sobre marcas, pessoas ou material de terceiros. O ZIP entregue não incluiu as pastas `sources/` nem `qa/` mencionadas no README do pacote; a integração usa os PNGs finais e o manifesto recebidos. O carrinho de compras foi conferido visualmente e seu último pixel de alpha está transparente, sem recorte na borda direita.

## Consequências

Os projetos anteriores preservam exatamente os IDs simples e continuam usando os desenhos originais. Novos projetos podem escolher entre forma procedural e arte ilustrada. Variantes incorporadas ao executável não dependem de caminhos de mídia externos. As cenas de baixa resolução funcionam tecnicamente, mas não prometem qualidade de entrega final.
