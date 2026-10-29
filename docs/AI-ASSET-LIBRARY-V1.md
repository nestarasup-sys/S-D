# AI Asset Library v1 — aparência padrão dos nativos

O pacote contém 48 objetos PNG transparentes e 12 cenários em resolução reduzida. Eles substituem a aparência padrão de 60 IDs nativos existentes; não criam uma biblioteca paralela. Uma receita deve continuar usando, por exemplo, `{"id":"carro","kind":"native","asset":"car","x":410,"y":315,"width":460,"height":210,"motion":"rise","motionFrames":24}`. `asset: "car"` já resolve para a arte PNG incorporada.

Os arquivos ficam embutidos no aplicativo em `:/studio/ai-library/v1/`. O renderizador resolve a arte centralmente pelo ID nativo e mantém cache de imagens decodificadas. A posição, as dimensões, a escala, a rotação, a opacidade, os keyframes, os grupos, a câmera e os movimentos externos continuam funcionando. Os PNGs dos 48 objetos e 12 cenários não suportam recoloração dos pixels. O apresentador `person` é uma substituição adicional sob o ID original e possui 12 imagens de pose com troca discreta sincronizável; suas cores também são fixas.

Os 12 cenários são prévias provisórias de aproximadamente 482–483×271–272 px. A biblioteca os marca como **PRÉVIA / BAIXA RESOLUÇÃO**. Arquivos futuros em Full HD podem substituir os mesmos caminhos mantendo os mesmos IDs. Para uma entrega Full HD, prefira usar os cenários como storyboard ou adicionar mídia real de alta resolução até a substituição das artes.

O apresentador usa os PNGs `neutral`, `smile`, `wave`, `point-right`, `point-left`, `explain`, `thinking`, `surprised`, `tablet`, `thumbs-up`, `walk-a` e `walk-b`. `pose: "point"` continua equivalente a `point-right`, e `pose: "worried"` a `thinking`. Em receitas v3, `poseKeyframes` agenda a troca de pose por `cueProgress` ou `wordIndex` com timestamp real. O objeto mantém sua identidade e transformações. A propriedade opcional `walk` alterna as duas poses de caminhada somente no intervalo declarado; keyframes existentes de posição devem deslocar o apresentador durante esse intervalo. `motion` continua sendo apenas entrada/saída do nó.

Permanecem proceduralmente renderizados os 11 objetos de dados/gráficos: `bars`, `chart-line`, `chart-pie`, `chart-donut`, `chart-area`, `timeline`, `flowchart`, `network`, `dashboard`, `progress-ring` e `data-table`. Eles preservam a capacidade programável e animada. Para os outros 48 objetos, 12 cenários e para o apresentador quando uma pose PNG não puder ser decodificada, o desenho C++ anterior existe somente como fallback interno; ele não aparece como escolha duplicada na biblioteca.

Versões do Studio Dark que chegaram a expor IDs temporários `illustrated:<id>` continuam compatíveis: esses IDs antigos resolvem para o mesmo recurso original e não são listados como itens separados. Para receitas novas, use somente os IDs nativos originais indicados em `nativeAssets` no guia gerado.

A arte foi fornecida pelo usuário como conteúdo gerado por IA. O pacote recomenda conferir direitos de uso antes de distribuição; não representa licença sobre marcas, produtos ou conteúdo de terceiros.

Consulte também [ADR 0035](architecture/0035-ai-illustrated-library-variants.md), [ADR 0036](architecture/0036-ai-presenter-pose-animation.md) e o [guia oficial para IA](GUIA-PARA-IA.md).
