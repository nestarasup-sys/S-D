# AI Asset Library v1 — aparência padrão dos nativos

O pacote contém 48 objetos PNG transparentes e 12 cenários em resolução reduzida. Eles substituem a aparência padrão de 60 IDs nativos existentes; não criam uma biblioteca paralela. Uma receita deve continuar usando, por exemplo, `{"id":"carro","kind":"native","asset":"car","x":410,"y":315,"width":460,"height":210,"motion":"rise","motionFrames":24}`. `asset: "car"` já resolve para a arte PNG incorporada.

Os arquivos ficam embutidos no aplicativo em `:/studio/ai-library/v1/`. O renderizador resolve a arte centralmente pelo ID nativo e mantém cache de imagens decodificadas. A posição, as dimensões, a escala, a rotação, a opacidade, os keyframes, os grupos, a câmera e os movimentos externos continuam funcionando. As imagens não suportam recoloração dos pixels, poses nem animação de partes internas. Propriedades antigas de cor ainda podem existir em projetos e receitas por compatibilidade, mas não recolorem esses PNGs.

Os 12 cenários são prévias provisórias de aproximadamente 482–483×271–272 px. A biblioteca os marca como **PRÉVIA / BAIXA RESOLUÇÃO**. Arquivos futuros em Full HD podem substituir os mesmos caminhos mantendo os mesmos IDs. Para uma entrega Full HD, prefira usar os cenários como storyboard ou adicionar mídia real de alta resolução até a substituição das artes.

Permanecem proceduralmente renderizados: o personagem `person`, que conserva poses, e os 11 objetos de dados/gráficos: `bars`, `chart-line`, `chart-pie`, `chart-donut`, `chart-area`, `timeline`, `flowchart`, `network`, `dashboard`, `progress-ring` e `data-table`. Esses elementos preservam a capacidade programável e animada. Para os outros 48 objetos e 12 cenários, o desenho C++ anterior existe somente como fallback interno caso a imagem incorporada não possa ser carregada; ele não aparece como escolha duplicada na biblioteca.

Versões do Studio Dark que chegaram a expor IDs temporários `illustrated:<id>` continuam compatíveis: esses IDs antigos resolvem para o mesmo recurso original e não são listados como itens separados. Para receitas novas, use somente os IDs nativos originais indicados em `nativeAssets` no guia gerado.

A arte foi fornecida pelo usuário como conteúdo gerado por IA. O pacote recomenda conferir direitos de uso antes de distribuição; não representa licença sobre marcas, produtos ou conteúdo de terceiros.

Consulte também [ADR 0035](architecture/0035-ai-illustrated-library-variants.md) e o [guia oficial para IA](GUIA-PARA-IA.md).
