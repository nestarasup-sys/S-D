# AI Asset Library v1 — integração no Studio Dark

O aplicativo registra 60 variantes fornecidas no pacote visual: 48 ilustrações PNG com alpha e 12 cenários de prévia rasterizados.

## Uso na receita

As variantes mantêm os IDs de origem no formato `illustrated:<nativeId>`. Exemplo válido:

```json
{"id":"chip-ilustrado","kind":"native","asset":"illustrated:chip","x":470,"y":190,"width":320,"height":240,"motion":"rise","motionFrames":24}
```

O ID `chip` sem o prefixo continua usando o nativo procedural antigo. Variantes PNG aceitam posição, dimensão, escala, rotação, opacidade, movimentos de nó, keyframes, grupos e câmera. Elas não aceitam `color`, `secondaryColor`, `palette` nem `pose`; esses campos não recolorem ou alteram partes internas e são rejeitados pelo parser. A arte PNG permanece estática internamente.

## Prévia de cenários

Os IDs ilustrados de cenário são `illustrated:street`, `illustrated:office`, `illustrated:showroom`, `illustrated:studio`, `illustrated:night-city`, `illustrated:server-room`, `illustrated:newsroom`, `illustrated:laboratory-scene`, `illustrated:map-room`, `illustrated:financial-district`, `illustrated:cyber-grid` e `illustrated:archive-room`. Todos têm entre 482 e 483 pixels de largura por 271 ou 272 de altura e recebem o selo **BAIXA RESOLUÇÃO** na biblioteca. Servem para storyboard e prévia; para Full HD, use o nativo procedural do mesmo ID simples ou uma imagem/vídeo de maior resolução.

## Empacotamento e compatibilidade

Os PNGs são embutidos no aplicativo; o projeto armazena o ID da variante e não um caminho externo. O compositor usa o QPainter e tenta retornar ao desenho procedural correspondente se uma imagem não puder ser decodificada. Projetos com IDs antigos não mudam de aparência. Variantes podem ser escolhidas pela biblioteca ou em receitas IA. A lista `nativeAssets` continua com 72 itens procedurais; `illustratedAssets` apresenta as 60 variantes rasterizadas separadamente.

O pacote declara arte gerada pelo usuário. Essa informação de origem não concede automaticamente direitos de marcas ou de materiais de terceiros. O ZIP recebido não continha `sources/` ou `qa/`, apesar de mencionados no README que veio junto.

Consulte também [ADR 0035](architecture/0035-ai-illustrated-library-variants.md) e o [guia oficial para IA](GUIA-PARA-IA.md).
