# Direção de roteiro dinâmico — Studio Dark

Guia criativo complementar ao [guia técnico para IA](GUIA-PARA-IA.md). Versão 1.3, outubro de 2026. Esta página orienta composição e ritmo; o guia técnico define os campos JSON aceitos.

## Identidade editorial — investigação visual cinematográfica

Esta diretriz é uma **preferência criativa do canal**, não uma nova função do parser. Objetivo: vídeos informativos e investigativos com alta clareza, curiosidade sustentada, provas apresentadas com honestidade e montagem visual variada. Os recursos da biblioteca devem **mostrar algo que a narração ainda não conseguiria transmitir sozinha**; não devem funcionar como apresentação de slides com título, subtítulo, ícone e legenda repetidos.

### A unidade narrativa: voz + revelação visual

Planeje cada trecho a partir de cinco perguntas:

1. **O que a voz revela agora?** Identifique uma única descoberta, explicação ou relação por beat.
2. **Qual dúvida o espectador tem nesse instante?** Uma pergunta real, não suspense artificial.
3. **O que a tela demonstra?** Processo, evidência, trajetória, comparação, escala, localização ou consequência.
4. **O que muda no quadro e por quê?** Um detalhe ganha foco, um componente abre, o fluxo atravessa uma tubulação, a câmera encontra um alvo, uma fonte é ampliada.
5. **Como sabemos disso?** Fonte verificável, dado contextualizado ou rótulo claro de hipótese, exemplo ilustrativo ou reconstrução.

Se a tela só repetir a frase narrada em letras grandes, substitua por uma ação visual ou deixe um silêncio visual intencional. O espectador deve conseguir acompanhar a lógica mesmo quando a informação visual é não verbal.

### Ritmo, densidade e duração

Não existe regra universal de corte a cada três segundos. Use **~3 s sem mudança visual significativa como alerta editorial**, jamais como limite rígido para duração de plano, shot ou quadro. Um plano de sete segundos pode funcionar se evoluir internamente: enquadramento amplo → revelação do fluxo → detalhe crítico → pausa de compreensão. Um corte de um segundo pode ser útil para contraste, mas ruim para um dado que precisa ser lido.

Referências iniciais para experimentar, não obrigações:

| Função | Cadência ilustrativa | Intenção |
|---|---|---|
| Impacto ou contraste | fração de segundo a ~2 s | quebrar expectativa com informação real |
| Objeto ou ideia simples | ~2–4 s | localizar, reconhecer e avançar |
| Processo ou comparação | ~4–8 s ou mais | permitir evolução e causalidade |
| Documento, gráfico ou prova | tempo necessário à leitura | compreender origem, escala, unidade e implicação |
| Pausa após descoberta | variável | consolidar o significado, sem efeitos competitivos |

Um movimento permanente, sem foco narrativo, também vira ruído. Alterne velocidade, contraste, silêncio, detalhes, planos amplos e momentos estáveis. Cada corte, travelling ou efeito deve justificar sua existência.

### Texto na tela — excepcional e útil

- Dê preferência a **datas, valores com unidade, nomes, locais, fontes, citações curtas e palavras indispensáveis** para entender relações.
- Evite título/subtítulo de assunto repetidos em cada shot. A abertura pode ser tipográfica quando isso cria sentido; não é uma obrigação.
- Quando `captions` estiver ligado, evite também um segundo texto que copie a mesma frase. Considere a área segura das legendas.
- Para evidência longa, mostre primeiro a peça e depois destaque o trecho relevante, respeitando o tempo de leitura; nunca atribua ao documento dados que ele não contém.
- Textos em telas de dispositivos e molduras são conteúdo editável, mas sua existência só melhora o filme quando acrescenta informação.

### Gramática da investigação

| Momento da história | Resposta visual preferida |
|---|---|
| Pergunta ou suspeita | detalhe concreto que provoca a dúvida |
| Contexto | mapa, percurso, escala, linha causal ou situação |
| Prova | documento real, imagem fornecida, dado com fonte e unidade |
| Contradição | comparação de duas condições equivalentes |
| Mecanismo | animação gradual de processo, peça por peça |
| Consequência | retorno ao contexto com implicação visual clara |
| Conclusão | síntese visual e limite honesto do que é conhecido |

O gancho inicial deve apresentar uma pergunta verdadeira ou uma diferença intrigante. Não prometa revelações inexistentes, não use suspense para fabricar incerteza e não adie indefinidamente a resposta. Fatos contestados exigem contexto e atribuição.

### Repertório visual disponível na biblioteca atual

**Não substitua recursos antigos por novos nesta etapa.** Os 48 objetos PNG e os 12 cenários PNG continuam nos IDs nativos; o `person` usa 12 poses com `poseKeyframes` e ciclo `walk`; os 11 gráficos/dados seguem procedurais. Os 66 componentes adicionais `ui:*` têm categorias próprias:

- **Painéis editoriais:** conteúdo textual curto, fato verificável, comparação ou fonte; use `slotDefinitions` efetivos.
- **Dispositivos vazados:** imagem, texto ou vídeo nos slots mascarados declarados, sempre atrás da moldura; não invente perspectiva 3D automática.
- **Arquivo documental:** organize imagens e documentos reais fornecidos, com origem atribuída; moldura não transforma uma ilustração em prova.
- **Peças de cenário:** primeiro plano, plano intermediário e deslocamentos relativos para parallax **simulado em 2D**.
- **Op Art:** abertura, transição e textura gráfica usadas com moderação; não sobrepor linhas finas a textos críticos.
- **Equipamentos e processos:** explique como um sistema funciona, usando fluxos, revelações e relações entre objetos. Uma janela transparente só é animável como slot quando o manifesto a declara.

Todos os recursos rasterizados têm cores internas fixas e resolução finita. Os 12 cenários nativos atuais são prévias de baixa resolução: para zoom forte ou uso definitivo em tela cheia, prefira mídia real fornecida ou arte de maior resolução, se disponível.

### Do roteiro ao storyboard, antes do JSON

Para cada shot, registre internamente:

| Campo de planejamento | Exemplo de pergunta |
|---|---|
| Fala e descoberta | O que está sendo afirmado, e com qual grau de certeza? |
| Papel visual | É evidência, processo, contexto, comparação ou respiro? |
| Asset ou mídia | O ID existe no catálogo? O arquivo local existe? |
| Estado inicial | O que já está visível? A legenda o encobre? |
| Mudança no tempo | Que elemento, câmera ou slot muda ao longo da fala? |
| Sincronização | Que cue, cueProgress ou wordIndex real dispara a mudança? |
| Prova e revisão | Há fonte, unidade e contexto? É ilustração ou registro real? |

Depois traduza esse plano para a receita **v3** usando as estruturas reais de `shots`, `visualBeats`, `camera.path`, `keyframes` e `slots`. Não invente campos como "investigativeMode", "autoParallax" ou "evidenceLayer": são ideias de direção, não propriedades implementadas.

### Miniestudo — explicar um percurso sem repetir a voz

*Situação ilustrativa, sem alegação sobre uma empresa ou cidade real.* Voz: "Um resultado na estação descreve aquele ponto de coleta. A água ainda percorre a rede até chegar às casas."

1. Mostrar o ponto de coleta, **identificado como exemplo**; não fabricar um laudo real.
2. Revelar uma rota visual do ponto inicial à rede; a câmera segue o fluxo.
3. Aproximar um trecho da tubulação com destaque para a direção do percurso.
4. Terminar numa residência, fazendo a comparação entre local de medição e destino.
5. Se houver documento real fornecido, introduzi-lo com uma moldura `ui:archive-photo-frame` e citar sua origem. Sem documento, não exiba uma "prova" fictícia.

Essa sequência pode ocupar um shot mais longo com estados sucessivos ou vários shots curtos. O valor é a progressão, não o número de cortes.

### Áudio, tensão e pausas

Narração pt-BR natural, frases faláveis, pausas onde há inferência ou surpresa. Som reforça ações específicas; não coloque `sound` em cada corte por hábito. Use trilha, ducking e automação com moderação, sem competir com a inteligibilidade da voz. Para sincronizar, prefira `cueProgress` e `wordIndex` **quando o alinhamento real da voz estiver disponível**; nunca invente timestamps de palavras.

### Verificação antes de exportar

1. Cada trecho importante tem função visual além da decoração?
2. O ritmo alterna rapidez com tempo de leitura e compreensão?
3. Há algum plano estagnado sem intenção? Há algum corte rápido demais para entender?
4. Textos, captions, mãos do personagem, UI e elementos em primeiro plano se sobrepõem?
5. Máscaras, slots de imagem/vídeo e percursos de câmera ficam corretos na prévia e no MP4?
6. Datas, números, unidades e fontes foram verificados externamente? Uma reconstrução está claramente identificada?
7. O que o público pensa que está vendo corresponde ao que a imagem realmente é?

O `VisualQualityAnalyzer` do Studio Dark é **consultivo e parcial**: não verifica todos os frames, não conhece a intenção artística e não substitui checagem factual ou revisão humana. Confira primeiros quadros, beats, pausas, cortes, destinos de câmera, capturas de momentos importantes e o arquivo exportado.

## Diagnóstico de staging e câmera

Na receita original do Laboratório de Movimento, os pontos do travelling vertical e horizontal já ficavam dentro do enquadramento inicial e estavam visíveis ao mesmo tempo. A câmera percorria apenas uma fração da distância entre eles. Isso explica a sensação de elementos dispersos: a receita demonstrava deslocamento, mas não preparava uma revelação progressiva. A inspeção não apontou defeito geral na avaliação da câmera nem no layout automático do editor.

Para cada movimento, pergunte: qual informação está visível no começo, o que muda durante o trajeto e onde a câmera deve parar? Distribua os assuntos em enquadramentos distintos. Escolha explicitamente a **revelação espacial** (objeto já visível no espaço e fora do quadro até a câmera chegar), a **revelação temporal** (objeto oculto com `opacity: 0` até `show`) ou a combinação. Não use `show` para simular movimento espacial. Reserve margem para títulos, legendas e bordas seguras; não compacte todos os objetos no centro do canvas.

`camera.x` e `camera.y` indicam o centro do quadro em coordenadas da cena. Pan, zoom e rotação afetam todos os conteúdos do shot, inclusive textos comuns. As legendas automáticas são uma sobreposição ancorada ao viewport: mantêm a posição segura durante pan, zoom, rotação e transições. Não há uma camada genérica para fixar qualquer texto na tela. Se um título deve permanecer estático durante um travelling, coloque-o em outro shot ou mantenha a câmera parada enquanto ele está na tela.

## Gramática visual

### Cena didática

Mostre a técnica que está sendo explicada. Para um travelling vertical, empilhe pontos A, B e C fora das bordas do quadro inicial. Comece centrado em A; mova a câmera até B quando a narração introduzir a segunda ideia; depois avance a C. Deixe somente o ponto atual e um indício do próximo visíveis. Use uma forma simples para indicar direção e um título curto para nomear a técnica.

### Cena editorial

Organize a informação conforme a narrativa, não conforme a lista de capacidades do aplicativo. Alterne enquadramentos amplos, detalhes, objetos, imagens e pausas tipográficas. Use movimentos quando eles conectarem ideias, revelarem escala ou mudarem o foco. Evite aplicar a mesma entrada e o mesmo zoom a todos os elementos.

### Composição comparativa

Use dois lados somente quando houver duas condições reais a comparar. Mantenha cabeçalhos alinhados, unidades equivalentes e texto curto. O Comparison Pack contém placeholders demonstrativos; substitua-os por conteúdo verificado e identifique a fonte antes de publicar.

### Template premium

Os recursos premium são composições editáveis com layouts próprios em 1280×720, 1920×1080 e 1080×1920. Escolha o template pelo papel narrativo, não apenas pela aparência. Separe uma abertura tipográfica de um plano de objeto e de um dado comparativo. Veja [catálogo e status](premium/PREMIUM-CATALOG.md).

### Variantes ilustradas AI Library

Os IDs nativos originais já selecionam a arte padrão atual: use `car`, `chip` e os demais IDs diretamente. Para 48 objetos e 12 cenários, essa arte é PNG estática; ela aceita movimento, transformação, opacidade, grupos e câmera, mas não pose, recoloração de pixels ou animação de partes internas. Os cenários (`street`, `office`, `showroom`, `studio`, `night-city`, `server-room`, `newsroom`, `laboratory-scene`, `map-room`, `financial-district`, `cyber-grid` e `archive-room`) estão marcados como prévias de 482–483×271–272 px; use-os para storyboard/prévia ou acrescente mídia real em alta resolução até a substituição Full HD. O personagem `person` e os 11 objetos de gráficos/dados permanecem procedurais e dinâmicos. IDs temporários `illustrated:<id>` ainda abrem projetos anteriores, mas não devem ser usados em receitas novas.

## Câmera e movimentos

- **Travelling horizontal:** coloque os centros A, B e C em sequência no eixo X. Comece no centro de A; depois mova a câmera a cada mudança de ideia. `examples/camera-travelling-horizontal-v3.production.json` demonstra o percurso x=250 → 900 → 1550, mantendo os pontos futuros ocultos até seus beats.
- **Travelling vertical:** distribua os centros no eixo Y, não em uma linha que caiba inteira no enquadramento inicial. `examples/camera-travelling-vertical-v3.production.json` usa y=310 → 940 → 1570 com revelações progressivas.
- **Pan:** conecta duas áreas da cena. Se o destino já estiver plenamente visível no primeiro quadro, o movimento não produz uma revelação clara.
- **Zoom:** use valores próximos de 1 para preservar contexto e aumente o zoom para enfatizar um detalhe. Não combine grande zoom com deslocamento extenso sem conferir que o assunto permanece dentro do canvas.
- **Rotação:** reserve para situações em que a inclinação comunica instabilidade ou mudança de perspectiva. Pequenos ângulos são mais legíveis para conteúdo editorial.
- **Parallax simulado:** coloque fundo, meio e frente em grupos separados e mova-os com amplitudes diferentes. A câmera e as transformações são 2D; não há profundidade física automática.
- **Tracking:** o aplicativo oferece movimento de câmera dirigido por keyframes, presets e beats; não oferece rastreamento automático de um alvo por visão computacional. Anime o centro da câmera junto às posições do assunto.

Para sequenciar enquadramentos sem estimar centros e zoom manualmente, use `camera.path` no shot. A lista mantém a ordem declarada e cada ponto mira um ID de elemento ou grupo. O compilador mede bounds pelas transformações locais e grupos ancestrais, centraliza o alvo, calcula zoom com `padding`, respeita `safeMargin` e reserva `captionSafeBottom`; os estados resultantes são keyframes reais no projeto. O primeiro alvo precisa coincidir com o corte, e os tempos precisam crescer. Os alvos podem usar `reveal: "spatial"` (padrão), `"temporal"` ou `"combined"`. Para temporal/combinado, mantenha a opacidade inicial efetiva em zero e inclua `show`; para espacial, deixe visível e não use `show` para o mesmo alvo. Use `motion: "none"` quando o fade padrão impedir que um objeto espacial já esteja visível no começo do trajeto.

Use `cameraPresets` ou Visual Beats de câmera em cada shot, sem combinar os dois no mesmo shot. Para movimento definido em relação à narração, prefira beats com `cueProgress` ou `wordIndex` alinhado pela voz real. Não estime tempo de palavra pelo texto.

## Revelações e ritmo

- Comece o objeto futuro com `opacity: 0`; `show` o revela no frame do beat e o mantém visível. `hide` oculta nesse frame; outro `show` pode reapresentá-lo.
- Em uma sequência, revele uma camada ou um pequeno conjunto relacionado por vez. Não ligue todos os elementos a um único beat se o objetivo é construir uma ideia progressivamente.
- Use `transform` com duração para mudança gradual de posição, escala ou opacidade. Use `show`/`hide` para cortes instantâneos de visibilidade.
- Reproduza a fixture [revelação sequencial](../examples/visual-beats-sequential-reveal-v3.production.json) para um exemplo executável com três elementos.
- Deixe tempo de leitura após uma revelação. Movimento contínuo durante toda a fala reduz a clareza e pode dificultar a leitura das legendas.

## Variedade de composição

1. **Tipografia cinética:** alterne título isolado, palavra-chave e texto explicativo. Preserve contraste e uma área para legendas.
2. **Infográfico:** revele a pergunta, depois os elementos ou valores, depois a conclusão. `bars`, `chart-line`, `chart-pie`, `chart-donut`, `timeline`, `network` e `data-table` são ilustrações; não recebem dados verificados automaticamente.
3. **Objeto e contexto:** associe um nativo, premium ou mídia real a uma frase curta. O objeto deve esclarecer a ideia, não decorar sem função.
4. **Quatro direções:** faça slides entrarem pela esquerda, direita, cima ou baixo quando a direção corresponder à mudança de assunto. Não use as quatro direções no mesmo shot sem necessidade.
5. **Pausa:** um quadro parado antes ou depois de uma revelação pode dar mais ênfase do que outra animação.

Use easing como direção de aceleração: `linear` para movimento constante, `ease-in` para acelerar, `ease-out` para desacelerar, `ease-in-out` para iniciar e terminar suavemente. A curva do keyframe de destino controla o trecho que termina nele.

## Apresentador ilustrado

Use o ID nativo existente `person`; a arte padrão é o apresentador PNG em 12 poses, sem item duplicado na biblioteca. `pose` escolhe o estado inicial. Em receitas v3, `poseKeyframes` troca a pose no frame sincronizado por `cueProgress` ou por `wordIndex` quando a voz local fornece alinhamento real. A pose permanece até o próximo evento.

`motion` continua sendo a animação de entrada/saída e não inicia a caminhada. `walk` alterna `walk-a` e `walk-b` apenas entre `start` e `end`. Para andar pelo cenário, anime a posição do mesmo nó com `keyframes` dentro desse intervalo; configure `stopPose` para o quadro após a caminhada. Use o exemplo executável [personagem, poses e caminhada](../examples/presenter-poses-walk-cycle-v3.production.json). Os PNGs mantêm cores fixas e não são recoloridos por `color`.

## Cinco operações temporais que não devem ser confundidas

- `transitionIn` define como um shot entra em relação ao anterior.
- `motion` define a entrada e a saída padrão de um elemento.
- `keyframes` alteram propriedades do elemento ao longo de um intervalo.
- Visual Beats executam `show`, `hide`, `transform`, `camera` ou `sound` em momentos sincronizados à fala.
- Movimento de câmera altera o enquadramento compartilhado do shot inteiro.

## Direção de voz e exemplo

Divida a narração em trechos curtos e faláveis. Primeiro escreva o que o espectador deve descobrir; depois escolha quando e como cada camada entra. Não use os placeholders `00,0` dos templates como fatos. Consulte o contrato JSON, IDs e tokens de cor em [GUIA-PARA-IA.md](GUIA-PARA-IA.md).

Exemplos pequenos e validados pelo parser:

- [Travelling horizontal](../examples/camera-travelling-horizontal-v3.production.json)
- [Travelling vertical](../examples/camera-travelling-vertical-v3.production.json)
- [Travelling espacial com enquadramento assistido](../examples/camera-path-spatial-v3.production.json)
- [Travelling combinado com Visual Beat](../examples/camera-path-combined-v3.production.json)
- [Revelação sequencial](../examples/visual-beats-sequential-reveal-v3.production.json)
- [Premium com paleta JSON](../examples/premium-palette-v2.production.json)
- [Dramatic Quote personalizado](../examples/premium-dramatic-quote-slots-v2.production.json)
- [Comparison Pack personalizado](../examples/premium-comparison-slots-v2.production.json)
- [Smartphone com texto, imagem e paleta](../examples/premium-smartphone-slots-v2.production.json)
- [Question Hook declarativo](../examples/premium-question-hook-slots-v2.production.json)
- [Statistic Callout declarativo](../examples/premium-statistic-callout-slots-v2.production.json)

- [Apresentador com poses e caminhada](../examples/presenter-poses-walk-cycle-v3.production.json)
- [66 componentes de UI, texto e slots mascarados](../examples/ui-components-additional-v1.production.json)
- [Demonstração integrada da Etapa 14](../examples/premium-factory-stage14-demo-v3.production.json)
- [Demonstração combinada da Premium Factory](../examples/premium-factory-motion-showcase-v3.production.json)

Antes de exportar, revise a composição no primeiro quadro, nos quadros imediatamente antes/depois de cada beat e no destino de cada movimento de câmera. Confira paisagem e retrato quando o template for usado nos dois formatos.

## Revisão automática antes da exportação

No editor, **Qualidade visual** abre uma checagem consultiva; **Exportar MP4** abre a mesma checagem antes do seletor de arquivo. Ela procura cortes e overflow de texto, colisões entre textos/legendas, margens de papéis essenciais, contraste em texto sobre forma sólida, estado de beats `show` e enquadramento de alvos guardados por `camera.path`. O relatório também aparece em Logs como `visual-quality-last.json`. Um aviso não é bloqueio nem mudança automática: revise o frame e edite a composição conforme a intenção.

O analisador usa alguns frames de shots, alterações de câmera, slots de texto e Visual Beats. Ele não prova que cada frame está perfeito, não entende imagens ou intenção artística, não verifica fatos e pode não determinar se uma sobreposição decorativa é deliberada. A ausência de avisos não substitui preview e inspeção da exportação.

Os slots textuais com política declarada `textLayout` podem reorganizar a quebra e reduzir a fonte até o mínimo indicado no catálogo. Nenhuma palavra é apagada ou encurtada; se o conteúdo exceder até o mínimo, corrija o tamanho/quantidade de texto ou edite o template. Para slot de imagem do Smartphone, use apenas arquivo local relativo à pasta do `.production.json`; caminhos absolutos, URLs, UNC, traversal e symlink que saia dessa pasta são recusados. Após importação, a imagem é copiada para mídia gerenciada do workspace. O projeto ainda depende desse workspace até que uma função de empacotamento seja usada.

Para uma tarefa autocontida destinada a outra IA, use o [pacote de avaliação criativa](ai-evaluation/README.md). Ele pede uma receita com premium, slots, paletas, travelling espacial e reveal temporal, e explica como importar e registrar os avisos. A etapa de avaliação externa depende de o usuário executar esse desafio; nenhum serviço externo foi invocado.
