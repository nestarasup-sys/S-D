# Futuro do Studio Dark — direção audiovisual livre por IA

Data: 24/10/2026. Referência analisada: **Gravação de Tela 2026-10-24 170027.mp4**, enviada pelo usuário.

**Estado deste documento: proposta de evolução.** As capacidades futuras descritas aqui ainda não estão implementadas no protótipo 0.3. Esta entrega analisa a referência e define prioridades; não modifica o funcionamento do aplicativo.

## 1. A direção que recomendo

Evoluir o Studio Dark para um **estúdio de composição audiovisual que a IA consegue dirigir por inteiro**, com cenas construídas a partir de elementos, grupos, mídia, documentos, animação e câmera.

A biblioteca passa a fornecer peças e composições reutilizáveis. A linguagem de composição permite à IA criar combinações novas, inclusive quando não existe um modelo pronto para a cena imaginada.

O objetivo pode ser expresso assim:

> Eu forneço um tema ou roteiro e o guia do aplicativo. A IA escreve ou organiza a narrativa, concebe os visuais, escolhe enquadramentos, cria animações e descreve a produção. O aplicativo transforma essa direção em um projeto audiovisual completo, no qual eu consigo alterar cada parte.

Minha prioridade para a próxima etapa é **grupos + animação por propriedade + câmera 2D + documentos reais + vídeo dentro das composições**. Esse conjunto abre as possibilidades mais importantes da referência. A expansão do catálogo acompanha essa fundação.

## 2. Como a referência foi analisada

- Duração do arquivo: aproximadamente **45,50 segundos**.
- Imagem: **1918 × 1078**, 30 FPS; trata-se de uma gravação de tela próxima de Full HD, não de uma especificação obrigatória para nossos projetos.
- Há uma faixa de áudio AAC estéreo a 48 kHz.
- Foram inspecionados 23 quadros em intervalos de dois segundos, dois quadros na resolução original e quatro sequências com amostragem a cada meio segundo: abertura, quadro de conexões, destaque documental e linha do tempo.
- Uma transcrição automática local auxiliou a relacionar a narração às mudanças de assunto. Ela contém erros e não foi usada como fonte de citações, dados ou comprovação factual.
- Textos, comandos e interfaces que aparecem na gravação foram tratados apenas como conteúdo do vídeo, nunca como instruções para o agente ou o aplicativo.

O arquivo original permaneceu no local fornecido. Quadros e material de análise estão em `artifacts/reference-analysis/`, ignorado pelo Git. O vídeo e suas imagens não foram enviados a um serviço de transcrição; a transcrição auxiliar foi executada localmente.

Identificação do arquivo analisado — SHA-256: `D807649BE18A68D1787E6A2CFA5ED6D27C7D60A0985D4756C56D8289DA512405`.

**Limites da análise:** os tempos abaixo são aproximados. A aparência não permite identificar com segurança o software utilizado, a existência de uma cena 3D real ou o método original de animação. Não houve escuta crítica isolada de música, efeitos ou mixagem; propostas de áudio neste documento são recomendações de produto. Dados e notícias mostrados na referência não foram verificados.

## 3. O que o vídeo faz visualmente

| Trecho aproximado | Observação na referência | Capacidade que precisamos oferecer |
| --- | --- | --- |
| 00–03 s | Uma janela de terminal ocupa a tela; o enquadramento se afasta e revela cartões conectados abaixo. Há mudanças no estado visual de botões e cursor. | Grupos, estados de componentes, conectores e câmera que revela contexto. |
| 04–09 s | Números, pequenos marcadores, imagens/vídeos e cartões de pessoas aparecem relacionados a trechos de documentos. | Composição de mídias, contadores, matrizes de pontos, layouts e anotações. |
| 10–13 s | Um gráfico é mostrado em destaque e depois passa a integrar uma composição maior com ano e contador. | Reaproveitar um elemento no mesmo espaço e mudar o enquadramento sem reconstruir a cena. |
| 14–19 s | Manchetes e recortes ganham destaque; uma palavra grande assume temporariamente o foco da composição. | Recortes, hierarquia tipográfica, destaque seletivo e mudança de ênfase. |
| 20–27 s | A visualização percorre um conjunto de documentos, cartões e linhas; um enquadramento mais aberto revela as relações entre eles. | Quadro de investigação com coordenadas próprias, grupos e câmera guiada por alvos. |
| 28–33 s | Um trecho de palestra aparece dentro de uma moldura; o enquadramento se aproxima e entram identificação e etiquetas. | Vídeo como camada editável, corte de origem, enquadramento e identificação reutilizável. |
| 33,5–36,5 s | Uma publicação aparece inteira; depois há aproximação, escurecimento do contexto e realce de trechos específicos. | Documento com regiões anotadas, máscara de foco e câmera vinculada ao trecho. |
| 37–40 s | Uma gravação de interface integra uma sequência de etapas conectadas. A visão se abre para mostrar o processo. | Vídeo dentro de cartões e diagramas animados, preservando o vínculo entre elementos. |
| 41–45 s | Uma linha do tempo associa datas, cartões e notícias; o enquadramento percorre e reposiciona a informação. | Eixo temporal editável, eventos ancorados e câmera sobre uma composição extensa. |

### A característica mais importante: continuidade espacial

A referência cria a sensação de que informações pertencem a um espaço conectado. Um cartão pode ser mostrado de perto, depois reaparecer em uma visão ampla junto de outros cartões. Isso ajuda o espectador a entender relações.

Recomendo produzir esse comportamento com um espaço de composição maior que a tela e uma câmera que determina o que será visto. Um plano pode mostrar o quadro inteiro; outro pode aproximar uma frase. A posição e a identidade dos elementos continuam preservadas.

### Hierarquia e acabamento

Observei fundo escuro, bordas luminosas, cores de ênfase, tipografia condensada em chamadas, texto monoespaçado em rótulos e linhas conectando informações. Há partículas/textura aparentes, sombras e uma assinatura visual recorrente.

O padrão de leitura alterna contexto amplo e detalhe. A densidade visual é alta em alguns momentos, mas o destaque e o enquadramento indicam onde olhar. Para nosso produto, precisamos dar tempo de leitura ao elemento principal e controlar o que fica em segundo plano.

Essa linguagem pode virar um **kit visual investigativo configurável**. Paleta, fontes, molduras, textura e intensidade dos efeitos devem poder mudar. Outros estilos, como explicação vetorial, tipografia e apresentação de produto, poderão usar a mesma estrutura.

## 4. O que a liberdade da IA significa no produto

A IA deve poder decidir:

- Qual é a melhor sequência narrativa e quais informações precisam aparecer juntas.
- Quando usar documento, gráfico, fotografia, vídeo, personagem, forma ou uma composição original.
- Como construir um objeto com formas, caminhos vetoriais, texto e grupos.
- Quais elementos se relacionam e como representar essa relação.
- Onde cada elemento fica, como entra, como muda e quando sai.
- Qual será o enquadramento e como a câmera conduz a atenção.
- O que deve reagir a uma frase, palavra, evento visual ou batida disponível no áudio.

O aplicativo deve executar operações definidas e verificáveis. A liberdade criativa vem da combinação dessas operações e da possibilidade de ampliar os tipos de elemento.

Por exemplo: mesmo sem um preset de “dossiê de uma concessionária”, a IA poderia combinar uma moldura, uma fotografia, uma etiqueta de preço, linhas, uma ficha e uma câmera. A cena resultante continua editável.

Quando faltar uma mídia, a IA pode indicar o que precisa ser fornecido, propor uma construção vetorial possível ou usar um gerador/provedor que esteja configurado. Uma descrição de imagem não pode ser tratada como se o arquivo já existisse.

## 5. Onde estamos hoje e o que precisa evoluir

| Área | Protótipo 0.3 | Evolução recomendada |
| --- | --- | --- |
| Projetos e coleção | Tela inicial, cópias, renomeação, lixeira, categorias e biblioteca local. | Preservar essa base e ampliar os tipos de item reutilizável. |
| Objetos | Nós em uma lista; nativos são desenhados internamente como um objeto parametrizado. | Árvore de elementos com filhos, grupos, âncoras e componentes editáveis. |
| Biblioteca | 14 nativos, imagens/SVG e variações de um objeto. | Elementos, grupos, composições, materiais, movimentos e kits de identidade. |
| Composição | O compilador de produção acrescenta cenário, cabeçalho, título e legendas em posições definidas. | Layout livre; títulos, cabeçalhos e legendas opcionais e posicionáveis. |
| Animação | Seis entradas, deslocamento e giro ao longo do intervalo. | Vários keyframes por propriedade, curvas, trajetórias e animações em etapas. |
| Câmera | Não há câmera global de composição. | Aproximar, afastar, percorrer e enquadrar grupos ou regiões. |
| Mídia | Imagem e SVG; vídeo de origem ainda não é uma camada do projeto. | Vídeo com trecho de origem, crop, escala, máscara, velocidade e áudio configurável. |
| Documentos | Imagem importada ou objeto ilustrativo de documento. | Páginas reais, recortes, regiões destacadas, fonte e referência editorial. |
| Dados | Barras conceituais, sem valores estatísticos. | Gráficos e contadores alimentados por dados, unidades e fontes identificáveis. |
| Sincronização | Entrada vinculada ao trecho de voz e legendas por trecho medido. | Marcadores de frase/palavra quando houver alinhamento real e confiável. |
| Som | Narração e mix final com música/efeitos procedurais. | Pistas separadas, envelopes, fades, efeitos pontuais e redução de música sob a voz. |
| Guia para IA | Contrato v1 e catálogo nativo estático. | Pacote gerado com capacidades, biblioteca ativa, estilos, mídia disponível e exemplos válidos. |

O piloto de carros comprovou o caminho entre receita, voz, projeto e MP4. A referência passa a orientar a próxima etapa de liberdade visual e direção.

## 6. Fundação de composição recomendada

### 6.1 Árvore de elementos

Um grupo reúne elementos e permite movê-los ou animá-los em conjunto. Seus filhos continuam editáveis separadamente.

```text
Composição: investigação
├── ambiente e textura
├── grupo: documento principal
│   ├── moldura
│   ├── página original
│   ├── máscara de contexto
│   ├── destaque do trecho
│   └── identificação da fonte
├── grupo: entrevista
│   ├── vídeo
│   └── identificação
├── conectores entre os grupos
├── câmera e seus planos
└── legendas / elementos da tela
```

Cada elemento precisa de ID estável, transformação local, intervalo, ordem visual e propriedades próprias. O pai determina a transformação do conjunto. Conectores devem apontar para âncoras de objetos e acompanhar seu movimento.

A distinção entre **espaço da composição** e **espaço da tela** é importante: documentos e cartões acompanham a câmera; uma legenda ou marca pode permanecer fixa na tela quando essa for a intenção.

### 6.2 Animação por propriedade

Suportar keyframes de posição, escala, rotação, opacidade e propriedades específicas do elemento. Incluir curvas de velocidade e trajetórias, com valores explícitos e avaliação determinística.

A IA poderá descrever uma sequência: entrar, aguardar, aproximar, destacar e sair. Ações podem se sobrepor; um deslocamento de câmera não precisa encerrar a animação de um cartão.

Presets de movimento serão atalhos que criam curvas editáveis. Um novo comportamento não deve exigir um preset adicional para cada objeto da biblioteca.

### 6.3 Câmera 2D e profundidade visual

Primeiro: posição, zoom, rotação moderada, enquadramento de alvo e transições suaves. A câmera poderá focar o grupo inteiro ou uma região de documento, respeitando margens.

Depois: paralaxe por camadas e perspectiva leve para gerar sensação de profundidade. A gravação não comprova uso de 3D real; podemos validar uma solução 2D/2,5D antes de assumir um motor 3D completo.

### 6.4 Elementos abertos à construção

O conjunto inicial deve incluir texto, retângulo, elipse, linha, caminho vetorial, imagem, vídeo, grupo, máscara e conector. Objetos compostos podem ser construídos com essas peças.

Os nativos atuais podem evoluir para fábricas de grupos: um carro expõe rodas, carroceria e janelas; um personagem expõe partes articuladas; um painel expõe moldura, mídia e rótulos. A IA recebe tanto os parâmetros práticos quanto acesso aos filhos quando necessário.

## 7. Documentos, vídeos e dados como material de direção

### Documentos reais e destaques

Importar uma página/imagem ou PDF, preservar a origem e permitir selecionar regiões. Um destaque precisa guardar sua área em coordenadas da página, acompanhando crop, escala e câmera.

Oferecer marca-texto, sublinhado, caixa de chamada, escurecimento do restante e aproximação da região. OCR pode ajudar a localizar texto; a posição e o trecho precisam ser conferíveis, com indicação de incerteza quando houver.

Uma fonte cadastrada é uma referência editorial, não uma verificação automática do que está sendo afirmado. A direção deve distinguir texto original, tradução, resumo e comentário do autor.

### Vídeos dentro das cenas

Um vídeo pode ocupar a tela, ficar dentro de uma moldura ou integrar um grupo com rótulos e conectores. Seu trecho de origem deve continuar separado de sua posição na montagem.

Precisamos definir trim, reprodução, congelamento de frame, preenchimento/encaixe, crop e destino do áudio de origem. Trocar o vídeo de um cartão deve preservar a moldura e as animações quando forem compatíveis.

### Gráficos e linhas do tempo

Adicionar barras, linhas, contadores e matrizes de pontos com dados explícitos. Um gráfico precisa de valores, unidade, escala e referência de origem; sua animação não pode alterar o significado dos números.

Linhas do tempo terão eventos com datas, rótulos, cartões e vínculos. A câmera poderá percorrer o eixo. Dados ilustrativos precisam continuar identificados como ilustrativos.

## 8. Biblioteca que sustenta liberdade

Recomendo organizar a biblioteca em camadas de complexidade:

| Camada | Conteúdo |
| --- | --- |
| Peças | Formas, caminhos, texto, imagens, vídeos e sons. |
| Objetos | Pessoas, carros, documentos, placas, equipamentos e outros grupos reutilizáveis. |
| Componentes editoriais | Cartão de fonte, entrevista, comparação, gráfico, linha do tempo e citação destacada. |
| Composições | Conjuntos de componentes, enquadramentos e animações que continuam abertos à edição. |
| Kits de identidade | Paletas, fontes, molduras, cores semânticas, tratamentos de imagem e intensidade dos efeitos. |

Cada item deve informar parâmetros, partes, âncoras, proporção, exemplos de uso e dependências de mídia/fontes. Uma miniatura sozinha não ensina a IA a utilizá-lo.

Salvar uma seleção deverá aceitar grupos inteiros e composições, além do nó individual que já existe. As categorias e a lixeira atuais podem continuar organizando esse material.

## 9. O novo guia para a IA

O pacote exportado pelo aplicativo deve conter:

1. Explicação do fluxo e da linguagem visual possível.
2. Schema da versão aceita e regras temporais.
3. Tipos de elemento, propriedades, operações e limites reais.
4. Manifesto da biblioteca ativa, incluindo itens personalizados e suas partes.
5. Mídias disponíveis, durações, dimensões, fontes e dependências pendentes.
6. Kit visual escolhido e exemplos válidos de composições variadas.
7. Regras para editar um projeto existente por IDs e revisão, preservando trabalho anterior.

O guia precisa ensinar a combinar e construir, além de enumerar itens. Os exemplos devem demonstrar soluções diferentes para o mesmo trecho de roteiro.

Três modos de uso:

- **A partir de tema:** a IA desenvolve roteiro e direção, separando pesquisa, afirmações e necessidades de mídia.
- **A partir de roteiro:** a IA trabalha a direção visual, preservando o texto salvo autorização para reescrever.
- **A partir de projeto:** a IA altera partes identificadas, com uma proposta de mudanças sobre a revisão atual.

O editor recebe o plano e faz a validação estrutural. A intenção narrativa e criativa continua vindo da IA e do usuário. Composição não deve ser automaticamente substituída por um layout obrigatório em toda cena.

## 10. Formato de direção mais expressivo

Recomendo especificar uma **receita v2** e um modelo de projeto que comporte hierarquia, câmera e mídia. A numeração definitiva deve ser registrada quando o contrato for implementado. Projetos antigos devem continuar legíveis e visualmente preservados por migração testada ou caminho legado.

O plano autoral mantém vínculos semânticos, como “entrar na segunda frase”. Depois que a voz estiver disponível, o compilador resolve esses vínculos para tempos e frames concretos. Assim, a IA dirige sem inventar a duração da fala.

Exemplo conceitual de direção futura — **não é compatível com o importador v1 atual**:

```json
{
  "format": "studio-dark-production",
  "version": 2,
  "requires": ["groups", "camera-2d", "document-region"],
  "styleKit": "investigacao-autoral",
  "narration": [
    {"id": "fala-1", "text": "Vamos observar este documento."},
    {"id": "fala-2", "text": "Agora, acompanhe o trecho destacado."}
  ],
  "assets": [
    {"id": "pagina-1", "path": "midias/documento.png", "status": "required"}
  ],
  "composition": {
    "nodes": [
      {
        "id": "painel",
        "type": "group",
        "children": [
          {"id": "pagina", "type": "image", "assetId": "pagina-1"},
          {
            "id": "destaque",
            "type": "document-region",
            "target": "pagina",
            "rectNormalized": [0.12, 0.40, 0.72, 0.16],
            "revealAt": {"cue": "fala-2", "edge": "start"}
          }
        ]
      }
    ],
    "camera": {
      "shots": [
        {"focus": "painel", "at": {"cue": "fala-1", "edge": "start"}},
        {
          "focus": "destaque",
          "at": {"cue": "fala-2", "edge": "start"},
          "transition": {"type": "pan-zoom", "durationFrames": 18}
        }
      ]
    }
  }
}
```

O arquivo de mídia do exemplo é apenas uma dependência ilustrativa. O aplicativo deverá conferir sua existência. O usuário não precisa escrever esse JSON; ele será o contrato entre a IA e o aplicativo.

Se uma capacidade ainda não existir, a importação deve apresentar a incompatibilidade e opções concretas de adaptação. Uma extensão instalada pode acrescentar capacidades registradas; o plano de produção continua declarativo.

## 11. Sincronização, som e ritmo

Preservar a medição real da voz que já temos. Acrescentar marcadores por frase e, quando disponíveis, por palavra, com identificação de sua origem e confiança.

Eventos podem controlar destaque de texto, entrada de cartão, mudança de contador ou foco da câmera. Posições por palavra precisam vir do sintetizador ou de alinhamento do áudio real. O Windows oferece eventos de progresso com posição no áudio; sua aplicação ao nosso pipeline precisa de um teste próprio, com fallback por trecho quando não houver informação confiável. [Referência: Microsoft AudioPosition](https://learn.microsoft.com/en-us/dotnet/api/system.speech.synthesis.speakprogresseventargs.audioposition).

Separar voz, música, efeitos e áudio de vídeos. Permitir ajustar volume, fades e redução de música sob a voz. A direção da IA poderá indicar intenções sonoras e eventos pontuais. A disponibilidade do som deve ser validada como a de qualquer outra mídia.

Legendas terão modos configuráveis: por frase, por palavra quando alinhada, seletivas ou desativadas. Citações e documentos já podem carregar muito texto; o desenho da cena deve considerar essa ocupação.

## 12. Arquitetura e desempenho

Manter C++ e Qt como base. Preservar projetos, biblioteca, IDs, revisões, recuperação e separação dos processos de produção/render que já funcionam.

O ponto central será um avaliador compartilhado: dado um projeto e um instante, ele resolve hierarquia, animação, câmera e referências de mídia. Prévia e exportação devem obedecer às mesmas regras de composição.

### Validar a evolução do renderizador antes de ampliar os efeitos

A prévia atual usa `QQuickPaintedItem`, e a composição atual usa `QPainter` sobre imagem. A documentação do Qt 6.8 descreve a rasterização em imagem seguida do envio à textura. Isso torna pertinente medir o custo de quadros grandes e efeitos; C++ por si só não comprova desempenho de composição. [Referência: QQuickPaintedItem 6.8](https://doc.qt.io/qt-6.8/qquickpainteditem.html).

Recomendo um protótipo comparando a composição atual com uma implementação baseada no Qt Quick Scene Graph. Esse sistema oferece transformações e geometria mantida para renderização gráfica. A escolha final deve resultar de medições no projeto e no hardware de destino. [Referência: Qt Quick Scene Graph](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html).

`QQuickRenderControl` é um candidato para avaliar renderização fora da tela sob controle do aplicativo. Sua existência não garante, sozinha, exportação determinística ou paridade com a prévia: relógio, fontes, shaders e disponibilidade dos frames precisam de validação. [Referência: QQuickRenderControl 6.8](https://doc.qt.io/qt-6.8/qquickrendercontrol.html).

Para vídeo, manter uma interface de acesso a frames separada do modelo. `QVideoSink` pode fornecer frames do Qt Multimedia para a prévia; a estratégia de decodificação/seek da exportação deve ser testada antes de assumir precisão por frame. FFmpeg pode continuar no processamento e codificação. [Referência: QVideoSink 6.8](https://doc.qt.io/qt-6.8/qvideosink.html).

Evitar dois sistemas independentes de animação: um para a janela e outro para o MP4. Podemos ter adaptadores de renderização, mas o cálculo de tempo e transformações deve permanecer comum.

Adicionar proxies de vídeo, cache de mídia/texto e descarte de elementos fora do enquadramento. Medir tempo de busca, resposta da interface, RAM, custo por frame e velocidade de exportação com uma cena representativa. Não há benchmark desse motor futuro nesta entrega.

## 13. Ordem de implementação recomendada

| Etapa | Entrega concreta | Critério de passagem |
| --- | --- | --- |
| 0 — Prova da fundação | Modelo mínimo de grupos, câmera e animação; comparação de caminhos de renderização com cena curta e mídia de teste. | Composição salva/reaberta, enquadramentos reproduzíveis, medições registradas e caminho de evolução escolhido. |
| 1 — Composição livre | Árvore de elementos, transformações locais, âncoras, seleção de grupos e keyframes básicos. Remover obrigatoriedade de cabeçalho/layout no contrato novo. | A IA constrói uma composição autoral e cada filho continua editável. |
| 2 — Câmera e conexões | Pan, zoom, enquadramento de alvos, transições e conectores que acompanham os objetos. | Mostrar um cartão de perto e depois o quadro completo sem perder relações ou identidade. |
| 3 — Documentos e vídeo | Vídeo com trecho de origem; páginas, crop, máscara de foco e regiões destacadas. | Palestra dentro de um cartão e aproximação de uma frase de documento funcionam na prévia e no MP4. |
| 4 — Dados e som | Gráficos com valores/unidades, contadores, linha do tempo e pistas de áudio. | Valores consistentes, fontes relacionadas e mix ajustável sem reconstruir os visuais. |
| 5 — Direção por IA v2 | Schema, guia gerado, biblioteca ativa, eventos de voz e propostas sobre projeto existente. | Roteiro externo vira direção variada; mudanças parciais preservam o restante do trabalho. |
| 6 — Acabamento e escala | Kit investigativo, sombras/brilhos/texturas moderados, proxies, cache e episódio maior. | Resultado autoral consistente, legível e medido no computador de destino. |

A especificação e os exemplos da receita v2 começam junto da etapa 0; a etapa 5 consolida o fluxo completo. Cada etapa deve deixar uma entrega demonstrável e um commit local. Datas dependem das provas técnicas, não foram estimadas aqui.

## 14. Primeiro piloto que aproxima a referência

Recomendo um vídeo de **15 a 25 segundos**, com roteiro curto, documento sintético identificado e mídia própria ou autorizada. Usar uma identidade original.

Sequência proposta:

1. Um cartão aparece em detalhe, com um título curto.
2. A câmera se afasta e revela dois cartões relacionados por uma linha.
3. Um cartão reproduz um trecho de vídeo dentro de uma moldura.
4. A câmera vai para uma página e destaca uma região junto da frase correspondente.
5. A visão se abre para uma comparação ou linha do tempo e encerra com uma ideia clara.

Critérios de aceite:

- A IA pode escrever a direção sem escolher um layout obrigatório para toda cena.
- Grupos, filhos, vídeo, trecho destacado, câmera e curvas são editáveis.
- Alterar um enquadramento preserva a narração e seus marcadores.
- Trocar uma mídia preserva as propriedades compatíveis do cartão.
- Fonte e dependências estão identificadas; material ausente é informado.
- Salvar e reabrir preservam a composição; desfazer/refazer inclui as operações novas.
- Frames e áudio do MP4 correspondem ao projeto avaliado, incluindo limites de cortes e transições.
- Textos destacados ficam legíveis durante tempo suficiente no enquadramento final.

Esse piloto deve ser concluído antes de expandir o catálogo de efeitos e modelos. Ele demonstra a habilidade de dirigir a atenção, que é a característica mais importante da referência para este projeto.

## 15. Decisões de produto que ficam registradas

- A IA externa é uma diretora audiovisual; o aplicativo fornece linguagem, biblioteca e execução.
- O usuário pode começar de um tema, roteiro ou projeto existente.
- A liberdade de construção depende de peças combináveis, grupos, animação e câmera.
- Biblioteca, interface manual e automação devem manipular o mesmo modelo editável.
- O estilo investigativo será um kit configurável, e outras linguagens visuais poderão compartilhar o motor.
- Os próximos investimentos principais são composição, câmera e mídia/documentos; a gestão de projetos e biblioteca da versão 0.3 continua sendo aproveitada.

Próxima ação recomendada: **especificar o modelo de grupos/câmera e a receita v2 mínima, e construir a prova técnica da etapa 0**. Este documento orienta essa implementação futura; não a declara concluída.

## 16. Progresso da implementação

**Primeiro corte iniciado em 24/10/2026:** projeto schema v3 com grupos hierárquicos, keyframes lineares de transformação e câmera 2D. Prévia e render worker usam o mesmo avaliador; seleção e arraste no preview convertem as coordenadas pela câmera e pela hierarquia. O projeto de demonstração já exercita grupo e movimento de câmera. Projetos v1/v2 existentes continuam legíveis.

**Controles do primeiro protótipo:** o inspetor cria um grupo em torno do elemento selecionado, grava keyframes no playhead e permite animar centro/zoom/rotação da câmera. Os valores transformáveis do objeto passam a editar o keyframe atual quando há uma trilha. A lista diferencia filhos; arrastar um grupo na timeline desloca sua árvore e suas chaves, e excluir um grupo remove toda a árvore. A interface agrupa um objeto por vez, sem multi seleção, e o salvamento de grupos inteiros na biblioteca ainda não está disponível.

Esse corte entrega o primeiro uso manual da fundação, mas ainda não conclui o marco 0: a comparação de desempenho entre renderizadores permanece pendente. Curvas de velocidade, multi seleção, conectores, salvar composições e a migração explícita de projetos antigos ficam para etapas seguintes. O build do desktop e dos workers concluiu em 24/10/2026; a suíte automatizada não foi executada nestes cortes.
