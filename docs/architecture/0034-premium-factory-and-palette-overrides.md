# ADR 0034 — Premium Factory e paletas por receita

Data: 26/10/2026. Estado: implementado no caminho oficial QPainter.

## Contexto

Três templates premium já eram construídos por funções específicas. Criar novos itens repetia configuração de nós, slots, superfícies e keyframes. As receitas também não tinham um contrato conciso para direcionar cores de recursos registrados. Em paralelo, demonstrações de câmera mostravam todos os pontos de interesse ao mesmo tempo, embora o movimento de câmera fosse executado corretamente.

## Decisões

- `PremiumFactory` em `modules/core/src/premium_factory.cpp` concentra construção de superfícies, filetes, textos semanticamente identificados e keyframes para presets de entrada. É uma biblioteca interna de código C++, não um interpretador de pacotes, linguagem de animação externa ou runtime de plugins.
- Quatro construtores reutilizam essa base: Dramatic Quote, Lower Third Editorial, Source Label e Number Emphasis. Os três premium anteriores permanecem registrados. Todos os sete recursos seguem usando nós normais e `CompositionPainter`/QPainter.
- Cada ID tem um manifesto premium v1 embutido. O registro C++ declara os tokens de paleta permitidos; a inicialização verifica que os tokens do manifesto correspondem ao registro. A validação existente também compara manifesto e árvore produzida pelo construtor.
- Receitas v2/v3 aceitam um objeto opcional `palette` na referência `kind: "library"` de um premium oficial. Cada valor deve ser uma cor `#RRGGBB` e cada token precisa estar declarado naquele manifesto e ter uma camada semântica correspondente. Tokens desconhecidos, cores malformadas e campos extras geram erro. Essa personalização é compilada em cores comuns dos nós e não muda o schema do projeto salvo.
- Elementos `kind: "native"` aceitam somente `palette.primary` e `palette.secondary`, que mapeiam para as cores principal e secundária do elemento. Não se aplica paleta a formas arbitrárias, presets pessoais ou pacotes de terceiros.
- As variantes de layout paisagem/retrato são decididas pelos construtores segundo o canvas recebido. A fábrica não representa um sistema de constraints adaptativo nem um layout editor visual.
- Staging de travelling é responsabilidade da cena: a câmera é centralizada no ponto atual, e pontos futuros são colocados fora do viewport ou ocultos até um Visual Beat. O motor não faz auto-layout nem tracking automático. As receitas de exemplo evidenciam essa prática.

## Capacidades e limites atuais

IDs oficiais implementados:

- `premium:smartphone-premium-01`
- `premium:title-reveal-01`
- `premium:comparison-pack-01`
- `premium:dramatic-quote-01`
- `premium:lower-third-editorial-01`
- `premium:source-label-01`
- `premium:number-emphasis-01`

O Smartphone oferece troca de imagem e slots textuais. Os outros novos templates oferecem slots textuais; não há slots premium de vídeo ou ícone. A personalização de slots em receita não está no contrato: após inserir, o usuário ainda pode editar os nós no editor. A paleta é aplicada durante a compilação da receita.

As paletas são fechadas por manifesto e aparecem em `premiumAssets[].paletteTokens` em `docs/capabilities-v1.json`. Para cores fora desses papéis ou para mídias reais, use os fluxos existentes do aplicativo. A paleta não é um sistema de CSS, gradientes ou materiais.

## Evidências e testes

`examples/premium-palette-v2.production.json`, as receitas horizontais/verticais e `examples/premium-factory-motion-showcase-v3.production.json` são fixtures pequenas do contrato real. Os testes de produção passam esses arquivos pelo parser e compilador, conferem as cores resultantes e renderizam um quadro. `premium_templates_qpainter_profiles` verifica cada premium nos canvases 1280×720, 1920×1080 e 1080×1920. A demonstração visual curta gera projeto, receita, storyboard, PNGs e MP4 localmente sob `artifacts/`, sem incluir arquivos pesados no repositório.

O diagnóstico do Laboratório de Movimento localizou o problema de encenação na receita: vários alvos já estavam visíveis dentro do primeiro enquadramento enquanto a câmera se deslocava apenas parcialmente. A correção é progressão espacial e ocultação/revelação explícita nas fixtures e no guia, não uma mudança na semântica da câmera. Esse diagnóstico não prova que toda composição gerada por IA será automaticamente bem dirigida.

## Consequências

Novos templates editoriais comuns podem compartilhar construção de nós e animações em vez de duplicar os detalhes mecânicos. A composição visual continua específica de cada construtor, para que os recursos não virem cartões genéricos. Manifestos e registro C++ podem divergir; a validação no startup e os testes tornam essa divergência visível.

O escopo atual mantém o backend GPU experimental e separado, não altera formato do projeto, não instala pacotes externos e não muda a exportação. O [catálogo premium](../premium/PREMIUM-CATALOG.md) marca sete itens como implementados e os demais 23 como planejados.
