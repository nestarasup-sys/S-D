# Studio Dark — guia para dirigir um vídeo com IA

Catálogo 1 · Receita 3 · Projeto 13 · Native Lab 0.4

Você é o diretor audiovisual. Transforme o roteiro em uma produção editável. Preserve o texto do usuário, salvo pedido explícito de reescrita. Escolha enquadramentos e composições livres usando somente os recursos abaixo. Entregue JSON puro, sem cercas Markdown.

## Fluxo

O usuário escreve o roteiro fora do aplicativo, fornece este guia à IA e salva a resposta em `.production.json`. **Importar produção IA** gera voz local, mede cada fala, resolve os cues, monta o projeto e sincroniza o áudio. A pessoa pode editar cada grupo e elemento no editor e exportar o vídeo. Não invente duração nem timestamps da fala.

Receitas v1 e v2 continuam aceitas por compatibilidade. Para novas produções com cortes dentro da mesma fala, use v3.

## Como planejar a receita

- Responda **somente com JSON válido**, sem comentários, cercas Markdown nem texto de apresentação. Use `format: "studio-dark-production"` e `catalogVersion: 1`; escolha `version: 3` para shots/Visual Beats ou `version: 2` para a composição de cena tradicional.
- Preserve o roteiro recebido, salvo instrução de reescrever. Divida cada cena em trechos faláveis, de no máximo 150 caracteres, e use `cue` como índice (a partir de zero) na narração da **mesma cena**.
- `offsetSeconds` varia de 0 a 10; `cueProgress` usa de 0 a 1 para acompanhar a fala final. Sem duração fixa, a duração natural depende da voz pt-BR e só é conhecida depois de sintetizar o WAV.
- Monte a narrativa com composição livre, `group`, `shape`, `text`, `native`, `image`, `svg`, `video`, biblioteca pessoal, câmera e keyframes documentados. Reserve espaço para as legendas se `captions` for `true`.
- Conte no máximo 200 nós, incluindo filhos, legendas automáticas, título/kicker e cenário. Sem `canvas`, use 1280×720; nos outros formatos aceitos, componha elementos dentro das dimensões escolhidas e mantenha margens para legendas. Respeite o limite de oito níveis de grupo.
- Nunca invente `assetId`, caminho de mídia, arquivo de vídeo, fonte jornalística ou valor numérico como se fosse real. Se faltar mídia, componha com formas e textos ou indique ao usuário o arquivo necessário.
- Revise antes de responder: JSON parseável, IDs únicos, no máximo 20 cenas, de 1 a 10 falas por cena, propriedades dentro dos limites e narração coerente com os visuais.

## Receita v2 — compatibilidade

```json
{
  "format": "studio-dark-production",
  "version": 2,
  "catalogVersion": 1,
  "title": "Uma história visual",
  "voice": "auto",
  "speechRate": 0,
  "captions": true,
  "musicGain": 0.025,
  "effectsGain": 0.035,
  "sources": [],
  "scenes": [{
    "id": "abertura",
    "narration": ["Primeira frase.", "Segunda frase."],
    "elements": [{
      "id": "painel",
      "kind": "group",
      "x": 180, "y": 150, "width": 900, "height": 450,
      "cue": 0,
      "children": [
        {"id": "cartao", "kind": "shape", "x": 0, "y": 0, "width": 700, "height": 360,
         "color": "#17242c", "cue": 0,
         "keyframes": [
           {"cue": 0, "x": -40, "y": 0, "opacity": 0, "scale": 0.96},
           {"cue": 0, "offsetSeconds": 0.5, "x": 0, "y": 0, "opacity": 1, "scale": 1, "easing": "ease-out"}
         ]},
        {"id": "titulo", "kind": "text", "text": "A história por trás do preço",
         "x": 36, "y": 40, "width": 620, "height": 110, "fontSize": 48, "cue": 0},
        {"id": "carro", "kind": "native", "asset": "car",
         "x": 420, "y": 175, "width": 230, "height": 105, "cue": 1}
      ]
    }],
    "camera": [
      {"cue": 0, "x": 640, "y": 360, "zoom": 1.25, "rotation": 0},
      {"cue": 1, "x": 640, "y": 360, "zoom": 1, "rotation": 0, "easing": "ease-in-out"}
    ]
  }]
}
```

Use de 1 a 20 cenas. Cada cena tem de 1 a 10 trechos de narração, até 150 caracteres cada. A produção aceita no máximo 200 elementos contando grupos e seus filhos. IDs de cena, grupo e objeto precisam ser únicos. Áudio por voz usa uma voz pt-BR instalada no Windows; `speechRate` vai de -3 a 3.

`captions` controla as legendas visuais automáticas (padrão `true`). O arquivo SRT ainda é criado mesmo quando `captions` é `false`. Quando a voz Windows pt-BR emite alinhamento real, a legenda visual destaca a palavra falada com `artDirection.primaryColor` (ou a cor primária padrão). O projeto guarda os intervalos em quadros no schema 7; isso não exige um campo adicional na receita. Depois da importação, o inspetor permite ajustar início e fim de cada palavra alinhada numericamente; na faixa da legenda da timeline, arraste um limite para aproximar ao quadro mais próximo. Limites compartilhados mantêm palavras vizinhas contíguas e um gesto inteiro pode ser desfeito. Esses ajustes ficam no projeto compilado e não são campos de entrada da receita. Sem alinhamento real, a legenda continua estática. O SRT não contém cores.

`wordAnimation` é opcional na raiz v3 e aplica movimento à palavra ativa de cada legenda alinhada. Presets: `highlight` (padrão, somente cor), `none`, `pop`, `rise`, `fade` e `pop-rise`. Exemplo: `{"wordAnimation":{"preset":"pop-rise","durationFrames":8,"startScale":0.82,"travelY":12}}`. `durationFrames` aceita 1–30 (padrão 8), `startScale` aceita 0.5–1.5 (padrão 0.82) e `travelY` aceita −80 a 80 px (padrão 12; positivo sobe). O app usa timestamps reais da voz e rejeita presets animados se não houver um timestamp por palavra. O projeto salva os parâmetros no schema 12; inspetor, prévia e exportação usam o mesmo preset. A escala e a subida terminam no tamanho/posição normal; `fade` aumenta a opacidade durante o intervalo inicial.

## Plano temporal: voz sincronizada e duração fixa

A receita v2 permite dois modos. **Natural** (padrão para projetos antigos): sintetiza os trechos, mede o áudio e monta as cenas com pausas automáticas. **Duração fixa**: planeja o número exato de quadros solicitado depois de medir a voz. Use estes campos na raiz:

```json
{
  "format": "studio-dark-production", "version": 2, "catalogVersion": 1,
  "title": "Explicação em 8 segundos",
  "timingMode": "fit-to-duration", "targetDurationSeconds": 8,
  "preserveVoicePitch": true,
  "scenes": [{
    "id": "explicacao", "targetDurationSeconds": 8,
    "narration": [
      {"text": "Vamos entender o conceito.", "targetDurationSeconds": 3.5},
      {"text": "Veja a resposta na tela.", "targetDurationSeconds": 4.5}
    ],
    "elements": [
      {"id": "cartao", "kind": "shape", "cue": 0, "cueProgress": 0.25,
       "x": 180, "y": 120, "width": 900, "height": 420}
    ],
    "camera": [
      {"cue": 0, "cueProgress": 0, "x": 640, "y": 360, "zoom": 1, "rotation": 0},
      {"cue": 1, "cueProgress": 0.5, "x": 760, "y": 360, "zoom": 1.2,
       "rotation": 0, "easing": "ease-in-out"}
    ]
  }]
}
```

**Campos:** `timingMode` aceita `natural` ou `fit-to-duration`; para o segundo, `targetDurationSeconds` é obrigatório (1–600 s), e `preserveVoicePitch` só aceita `true` nesta versão. Em cada cena, `targetDurationSeconds` é opcional; em cada entrada de `narration`, um objeto `{"text":"...","targetDurationSeconds":2.5}` reserva um intervalo **incluindo voz e pausa**. Strings antigas continuam funcionando. Se todas as cenas tiverem alvo, a soma arredondada a 30 FPS deve igualar o total. As cenas sem alvo recebem o tempo restante. Alvos de fala devem caber na cena.

O Studio Dark sintetiza cada áudio primeiro, mede amostras a 24 kHz e depois planeja intervalos inteiros de quadros (30 FPS). O `atempo` do FFmpeg acelera apenas a fala que não cabe, buscando preservar seu tom, com limite rígido de **1,15×**. Quando sobra tempo, insere silêncio como pausa; não estica a voz para preencher. Se a fala precisar acelerar mais de 15%, o app **rejeita o plano** e apresenta ID e duração mínima. Requer FFmpeg e voz pt-BR local no Windows.

Para sincronizar com fala ajustada, use `{"cue":0,"cueProgress":0.5}` no elemento, keyframe ou câmera. `cueProgress` vai de 0 (início da fala) a 1 (último quadro audível); 0,5 significa a metade temporal da **fala ajustada**, não da cena nem de sua pausa. Não misture `cueProgress` e `offsetSeconds` no mesmo objeto. O `offsetSeconds` de 0 a 10 continua disponível para deslocamentos absolutos. A receita v3 também aceita `wordIndex` onde documentado abaixo; a resolução vem dos eventos reais da voz pt-BR.

**Planejamento:** para 90 s, 2700 quadros a 30 FPS. A IA pode planejar mas não conhece a duração de uma voz local ainda não gerada. Evite silêncios excessivos; se o plano não couber, ajuste a narração ou os tempos. O aplicativo não inventa áudio para satisfazer qualquer número.

## Receita v3 — cenas, shots e Visual Beats

### Formato do quadro

Receitas v1/v2 e receitas v3 sem `canvas` continuam usando 1280×720. Uma receita v3 pode declarar `"canvas":{"width":1080,"height":1920}` para vídeo vertical ou escolher 1280×720 e 1920×1080 para paisagem. `canvas` é um objeto opcional da raiz v3; `width` e `height` são inteiros obrigatórios quando o objeto está presente e nenhum outro campo é aceito. São aceitos somente estes pares: 1280×720 (padrão), 1920×1080 e 1080×1920. Dimensões e coordenadas dos elementos são pixels do quadro escolhido. Câmeras sem centro explícito começam no centro do quadro. O projeto mantém a resolução escolhida na edição, prévia e exportação. Cenários nativos de 16:9 preservam a proporção e são cortados nas laterais para preencher o formato vertical. Exemplo completo: `examples/retrato-1080x1920-v3.production.json`.

Uma cena narrativa pode conter vários shots sem repetir nem interromper a narração. A versão 3 aceita as configurações globais da v2 e substitui `elements` e `camera` da cena por `shots`. Cada cena v3 exige de 1 a 64 shots; a produção aceita até 256 shots e 4096 Visual Beats. O primeiro shot começa junto ao início da cena. O início do shot seguinte encerra o anterior; o último termina com a cena. Assim, os planos cobrem a cena sem lacunas ou sobreposições.

Cada cena v3 também pode declarar `audioTracks` (até 32 no projeto). Uma pista usa `id`, `source` (arquivo de áudio local relativo ao JSON ou absoluto; também pode ser um vídeo local que contenha áudio), `start` (a mesma referência a cue/progresso/wordIndex), `gain` (0–4), `duckUnderVoice` e `duckAmount` (0–1). `name` é opcional. `sourceInSeconds` (0–3600, padrão 0) corta a fonte antes da reprodução; `playbackRate` (0,25–4, padrão 1) controla sua velocidade sem alterar a duração da pista na timeline. Em fontes de vídeo, a faixa de áudio 0 é usada. `fadeInFrames` e `fadeOutFrames` são opcionais (0–1800) e aplicam fades lineares ao começo/fim da pista; a soma deve caber no intervalo disponível. `gainAutomation` é opcional e aceita até 128 pontos `{"frame":0,"value":0.5,"easing":"linear"}`; o quadro é relativo ao início da pista, `value` multiplica o ganho base (0–4), e `easing` aceita `linear`, `ease-in`, `ease-out` ou `ease-in-out`. Os pontos devem estar em ordem crescente. Antes do primeiro e depois do último ponto, mantém-se o nível mais próximo. A pista começa no ponto indicado e segue até o fim da cena. `duckUnderVoice:true` usa o WAV de voz isolado como referência para reduzir a pista durante a fala; sem stem dedicado, usa `audioSource`. Quando a referência é WAV PCM mono a 24 kHz, prévia e exportação usam a mesma envoltória RMS por quadro. Outros formatos usam o sidechain FFmpeg na exportação; para sincronização idêntica à prévia, forneça um WAV compatível. Pistas ficam editáveis no projeto; na timeline é possível ajustar início/duração, ganho, trim e velocidade, adicionar/substituir pontos no playhead e desenhar o envelope: clique na faixa para criar um ponto ou arraste um ponto para ajustar posição e multiplicador (0–4). O easing escolhido no controle da pista é aplicado durante o gesto; a ação inteira pode ser desfeita de uma vez. Limpar a automação remove todos os pontos.

Cada shot exige `id`, `camera` e `elements`; `name`, `transitionIn` e `cameraPreset` são opcionais. Em ritmo manual, `start` também é obrigatório e usa `cue` e um único campo entre `cueProgress` (0–1), `progress` (alias compatível), `offsetSeconds` (0–10) ou `wordIndex` (índice da palavra, começando em zero). `wordIndex` usa os eventos de progresso emitidos pela voz Windows pt-BR e não pode ser combinado com outros deslocamentos. O tempo é resolvido sobre o áudio medido. Com `editing.rhythm:"narration"`, omita `start` de todos os shots; o compilador escolhe cortes depois de palavras alinhadas, favorecendo pontuação e pivôs de frase como “mas”, “porém” e “então”, enquanto equilibra a duração dos planos conforme `editing.intensity`. A seleção não inventa timestamps: toda fala da cena precisa ter um timestamp real por palavra e precisam existir pontos suficientes para acomodar todos os shots, a duração mínima de cada plano e suas transições. Se o alinhamento ou os limites não forem suficientes, a produção informa o motivo. Elementos do shot sem referência temporal explícita começam no corte calculado, inclusive filhos de grupos; horários explícitos de elementos, beats e keyframes continuam respeitados e precisam caber no intervalo do shot. Não misture início automático e manual na mesma receita. `transitionIn` pode ser a string legada `cut`, um objeto `{"type":"cut"}` ou um objeto de transição animada. Um `durationFrames` opcional junto a `type:"cut"` é aceito por compatibilidade e normalizado para corte imediato. O primeiro shot da cena sempre começa sem transição. Transições animadas sobrepõem visualmente o último quadro do shot anterior e o novo shot, sem alterar a duração da narração nem o tempo total da cena.

Exemplos de transição: `{"type":"fade","durationFrames":18}`, `{"type":"dissolve","durationFrames":12}`, `{"type":"slide","direction":"left","durationFrames":16}` ou `{"type":"zoom","durationFrames":12}`. `durationFrames` aceita de 2 a 300 quadros e precisa caber no shot de entrada. `slide` aceita direção `left`, `right`, `up` ou `down`. `fade` atravessa preto; `dissolve` mistura as imagens; `slide` move o novo shot; `zoom` aproxima o novo shot ao entrar. Prévia e exportação usam o mesmo compositor. A timeline destaca a faixa de transição.

`camera` aceita `x`, `y`, `zoom` e `rotation`; quando omitidos, o centro corresponde ao canvas escolhido, o zoom é 1 e a rotação é 0.

Presets reutilizáveis de câmera são definidos uma vez em `cameraPresets` na raiz v3 e referenciados com `cameraPreset` em qualquer shot. Há limite de 32 nomes e 512 keyframes nas definições. Cada nome usa letras, números, `_` ou `-` e aponta para 2–64 estados completos de câmera. `offsetSeconds` é relativo ao início do shot, começa em zero e avança pelo menos um quadro; cada estado também exige `x`, `y`, `zoom` (0.1–10), `rotation` (−360 a 360) e `easing` opcional (`linear`, `ease-in`, `ease-out` ou `ease-in-out`). `x` e `y` aceitam −10000 a 10000. Exemplo: `{"cameraPresets":{"push-in":[{"offsetSeconds":0,"x":640,"y":360,"zoom":1,"rotation":0},{"offsetSeconds":1.2,"x":640,"y":360,"zoom":1.25,"rotation":0,"easing":"ease-in-out"}]}}`, com `"cameraPreset":"push-in"` no shot. O estado de offset zero substitui a câmera inicial daquele shot; os demais valores viram keyframes editáveis no projeto. O último estado precisa caber no intervalo do shot. O mesmo preset pode ser usado em vários shots. Não combine com `camera.focus.follow:true` nem com Visual Beats de ação `camera` no mesmo shot. O compilador limita a 4096 keyframes expandidos na produção.

Para enquadrar automaticamente um elemento ou grupo, adicione `"focus":{"target":"ID-DO-ELEMENTO","padding":1.25}` dentro de `camera`. O alvo deve pertencer ao mesmo shot. O aplicativo transforma os limites locais através de todos os grupos ancestrais, centraliza a câmera nesse retângulo e calcula o zoom para mantê-lo dentro do quadro com a margem solicitada. `padding` aceita 1–4 e assume 1.25. Quando `focus` está presente, ele substitui o centro e o zoom explícitos; a rotação continua sendo usada. Com `"follow":true`, o aplicativo recalcula o enquadramento para cada quadro do shot e grava a trajetória como keyframes editáveis. `follow` assume false. Não combine esse modo com Visual Beats de ação `camera` no mesmo shot.

Elementos declarados dentro do shot pertencem apenas a ele e viram faixas visuais editáveis no projeto. Sem referência temporal explícita, começam no início do shot. Os elementos e grupos continuam usando os tipos e propriedades documentados abaixo.

`editing` é opcional na receita v3. `intensity` aceita inteiro de 0 a 100 e só preenche `motion`, `motionFrames` e `exitFrames` omitidos nos elementos: 0 desliga movimentos automáticos, 1–30 usa entradas discretas com fade, 31–70 usa entradas rise e 71–100 usa pop mais rápido. Campos explicitamente declarados pela IA não são substituídos. `rhythm` aceita `manual` (padrão) ou `narration`; o modo narration requer inícios de shot omitidos em toda a receita e alinhamento real de cada palavra. Ele seleciona cortes em limites de frases e pivôs de fala disponíveis, buscando equilíbrio entre ritmo e pontuação; a intensidade ajusta esse equilíbrio. Sem timestamps suficientes ou duração para distribuir os shots, a produção falha com uma mensagem específica. Sem o objeto `editing`, receitas v3 mantêm os defaults anteriores; receitas v1/v2 não aceitam essa opção.

`artDirection` é opcional na raiz da receita v3 e define o estilo global. Os mesmos campos podem ser informados parcialmente em cada cena (`scenes[].artDirection`) e shot (`scenes[].shots[].artDirection`). A herança é global → cena → shot; cada objeto local substitui apenas os campos presentes. Valores `color`, `secondaryColor` e `fontSize` explícitos nos elementos mantêm prioridade sobre os defaults de direção de arte. O estilo efetivo controla fundo, tipografia, cartões padrão, cabeçalhos, legendas e destaque da palavra. O inspetor permite escolher escopo global, cena atual ou shot atual. `backgroundColor`, `glowColor`, `primaryColor`, `secondaryColor`, `textColor`, `cardColor` e `captionBackgroundColor` usam `#RRGGBB`; `fontFamily` aceita nome de fonte instalado (até 120 caracteres); `titleSize` e `bodySize` aceitam 8–160 px; `captionSize` aceita 8–120 px; `captionPosition` aceita `top` ou `bottom`; `safeMargin` aceita 0–300 px. Fontes externas e presets visuais nomeados ainda não são carregados.

Exemplo de herança parcial (fragmento):
```json
{
  "artDirection": {"textColor":"#F4F7FA", "primaryColor":"#55E2C4"},
  "scenes": [{
    "id":"abertura",
    "artDirection":{"backgroundColor":"#101820"},
    "shots":[{
      "id":"detalhe",
      "artDirection":{"primaryColor":"#FFCB69"}
    }]
  }]
}
```
O shot usa o fundo da cena e o texto global; só a cor primária substitui o valor herdado.

Na timeline do projeto gerado, a faixa superior mostra os shots e os marcadores dos beats. O controle Zoom ajusta a ampliação temporal de 1× a 8×; use Ctrl+roda sobre a régua para aproximar no ponto do ponteiro e Shift+roda ou a barra horizontal para navegar. Régua, shots, marcadores, camadas e envelopes compartilham o mesmo intervalo visível; clique na régua para posicionar o playhead nesse trecho. Arraste a alça verde à esquerda de um shot para mover o corte e ajustar a duração dos shots vizinhos; os elementos, beats e keyframes de câmera acompanham o corte. A faixa âmbar mostra a sobreposição de entrada de cada transição; arraste sua alça âmbar final para ajustar o overlap de 2 a 300 quadros, limitado pela duração do shot. Os arrastes de corte ou overlap são desfeitos em um único passo. Nas camadas visuais, use Ctrl+clique para adicionar/remover clipes da seleção; arraste qualquer clipe selecionado para mover o conjunto pelo mesmo delta, preservando os espaçamentos e respeitando os limites de cena. Com **Snap ligado**, os clipes alinham ao playhead, limites de cena/shot, beats, outras camadas e início/fim das pistas de áudio; a guia âmbar aparece durante o encaixe e a tolerância acompanha o zoom. Desligue Snap para posicionamento livre. Na pista de áudio, arraste a alça ↔ no início do envelope para mover o clipe e a alça direita para ajustar o fim; o início e beats de som vinculados acompanham a faixa. O movimento e o trim formam uma única ação de desfazer/refazer. O inspetor permanece focado no item principal selecionado; a seleção múltipla não cria um comando JSON. O inspetor grava keyframes de câmera e edita transição/duração no shot ativo. O snapping é uma ferramenta da timeline, não um campo da receita JSON.

Um `beat` exige `id`, `at` e `action`. `at` usa a mesma referência de fala do início do shot e também aceita `wordIndex` para sincronizar com o início de uma palavra reconhecida pelo sintetizador local. O índice é zero-based dentro do trecho `cue`; índices fora do alinhamento real são rejeitados. `show` e `hide` exigem `target` e mudam a opacidade do elemento no instante indicado. `transform` exige `target` e ao menos um de `x`, `y`, `rotation`, `scale` ou `opacity`; `durationFrames` opcional (0–300) controla a duração da mudança. `camera` altera `x`, `y`, `rotation` e/ou `zoom`, pode animar por `durationFrames` e não usa `target`. Todos os alvos devem ser elementos daquele shot; beats fora do intervalo são rejeitados. Beats de câmera e transformações são compilados como keyframes para manter o projeto editável.

`sound` dispara um efeito de uma vez no quadro do beat. Informe `source` com caminho para arquivo local de áudio (relativo ao `.production.json` ou absoluto) e `gain` opcional de 0 a 4 (padrão 1); não use `target`, `durationFrames` nem propriedades visuais. Extensões aceitas: WAV, MP3, M4A, AAC, OGG, FLAC e WMA. O arquivo precisa existir na máquina. O efeito vira uma pista de áudio editável ligada ao marcador; mover ou excluir a pista mantém o marcador sincronizado. O limite compartilhado é de 32 pistas por produção, incluindo estes efeitos. Projetos com efeitos de Visual Beat usam schema 9; preview e exportação reutilizam o sistema normal de áudio.

Exemplo de sincronização por palavra: `{"cue":0,"wordIndex":7}`. A posição é medida pelo evento de progresso do sintetizador Windows depois da geração da voz (e ajustada junto com a fala quando há aceleração de duração fixa). Em shots v3, `wordIndex` também pode iniciar um elemento ou marcar um keyframe; beats seguem usando `at`. O índice é zero-based dentro do trecho `cue` e não pode ser combinado com `progress`, `cueProgress` ou `offsetSeconds`. Se o sintetizador não emitir timestamps ou o índice não existir, a produção é interrompida com erro; não há estimativa por duração média da palavra. Projetos já compilados guardam os eventos como quadros, sem exigir timestamps depois da importação.

Exemplo curto, válido e coberto por teste automatizado:

```json
{
  "format": "studio-dark-production",
  "version": 3,
  "catalogVersion": 1,
  "title": "O preço do carro",
  "editing": {"intensity": 65},
  "artDirection": {"backgroundColor":"#101820", "glowColor":"#29434a",
    "primaryColor":"#f2b84b", "secondaryColor":"#65d6c4", "textColor":"#fff3d6",
    "cardColor":"#24323b", "captionBackgroundColor":"#101820",
    "fontFamily":"Segoe UI", "titleSize":58, "bodySize":44,
    "captionSize":27, "captionPosition":"bottom", "safeMargin":32},
  "scenes": [{
    "id": "oferta",
    "narration": ["O carro parece barato. Mas há um detalhe escondido."],
    "shots": [
      {
        "id": "aberto", "name": "Carro", "start": {"cue": 0, "cueProgress": 0},
        "transitionIn": "cut",
        "camera": {"x": 640, "y": 360, "zoom": 1, "rotation": 0},
        "elements": [{"id": "carro", "kind": "native", "asset": "car",
          "x": 360, "y": 290, "width": 460, "height": 210, "cue": 0}]
      },
      {
        "id": "detalhe", "name": "Preço", "start": {"cue": 0, "cueProgress": 0.55},
        "transitionIn": {"type": "dissolve", "durationFrames": 12},
        "camera": {"x": 640, "y": 360, "zoom": 1.15, "rotation": 0},
        "elements": [{"id": "preco", "kind": "text", "text": "BARATO?",
          "x": 420, "y": 240, "width": 440, "height": 100, "fontSize": 68,
          "cue": 0, "cueProgress": 0.55}],
        "beats": [
          {"id": "revelar", "at": {"cue": 0, "wordIndex": 7},
            "action": "show", "target": "preco"},
          {"id": "aproximar", "at": {"cue": 0, "cueProgress": 0.85},
            "action": "camera", "durationFrames": 8, "zoom": 1.35}
        ]
      }
    ]
  }]
}
```

Uma demonstração com dois trechos de fala, três planos e beats de transformação está em `examples/shots-visual-beats-v3.production.json`.

## Exemplo profissional completo validado no aplicativo

A receita a seguir é o Director's Cut de 90 segundos: seis cenas, dezoito shots, vinte Visual Beats, um efeito sonoro local e timestamps resolvidos pela voz pt-BR. O smoke de produção sintetiza a narração, renderiza 2700 quadros e compara os quadros de shots/beats da exportação com a prévia. Copie também `examples/media/soft-chime.wav` para uma pasta `media` ao lado do JSON para manter o caminho relativo do efeito. O atualizador do guia insere aqui o arquivo de exemplo real, sem reconstruir ou inventar seus campos.

<!-- STUDIO-DARK-VALIDATED-EXAMPLE-BEGIN -->
```json
{
  "format": "studio-dark-production",
  "version": 3,
  "catalogVersion": 1,
  "title": "O carro barato que quase virou um erro | Director's Cut 90s",
  "voice": "auto",
  "speechRate": 0,
  "captions": true,
  "wordAnimation": {
    "preset": "pop-rise",
    "durationFrames": 8,
    "startScale": 0.82,
    "travelY": 12
  },
  "timingMode": "fit-to-duration",
  "targetDurationSeconds": 90,
  "preserveVoicePitch": true,
  "editing": {
    "intensity": 52
  },
  "artDirection": {
    "backgroundColor": "#0B1420",
    "glowColor": "#1E4052",
    "primaryColor": "#55E2C4",
    "secondaryColor": "#FFCB69",
    "textColor": "#F4F7FA",
    "cardColor": "#172838",
    "captionBackgroundColor": "#0B1420",
    "fontFamily": "Segoe UI",
    "titleSize": 68,
    "bodySize": 40,
    "captionSize": 28,
    "captionPosition": "bottom",
    "safeMargin": 36
  },
  "musicGain": 0.02,
  "effectsGain": 0.025,
  "scenes": [
    {
      "id": "oferta-perfeita",
      "narration": [
        "A oferta parecia perfeita: um carro bonito, pouco rodado, e uma parcela que cabia no orçamento.",
        "Só que o valor anunciado era a entrada de uma história financeira que ainda não tinha sido contada."
      ],
      "shots": [
        {
          "id": "oferta-aberta",
          "start": {
            "cue": 0,
            "cueProgress": 0
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1,
            "focus": {
              "target": "carro-anuncio",
              "padding": 1.8
            }
          },
          "elements": [
            {
              "id": "carro-anuncio",
              "kind": "native",
              "asset": "car",
              "x": 350,
              "y": 350,
              "width": 560,
              "height": 255,
              "color": "#55E2C4",
              "motion": "slide-right",
              "travelX": 24
            },
            {
              "id": "comprador-anuncio",
              "kind": "native",
              "asset": "person",
              "x": 980,
              "y": 260,
              "width": 145,
              "height": 290,
              "pose": "worried",
              "color": "#FFCB69"
            },
            {
              "id": "titulo-oferta",
              "kind": "text",
              "text": "PARECIA PERFEITO",
              "x": 100,
              "y": 115,
              "width": 1080,
              "height": 90,
              "fontSize": 54,
              "color": "#F4F7FA"
            }
          ],
          "beats": [
            {
              "id": "oferta-titulo-revela",
              "at": {
                "cue": 0,
                "wordIndex": 4
              },
              "action": "show",
              "target": "titulo-oferta"
            },
            {
              "id": "oferta-camera-abre",
              "at": {
                "cue": 0,
                "wordIndex": 4
              },
              "action": "camera",
              "durationFrames": 12,
              "zoom": 1.08
            }
          ]
        },
        {
          "id": "oferta-etiqueta",
          "start": {
            "cue": 0,
            "wordIndex": 6
          },
          "transitionIn": {
            "type": "dissolve",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1.12,
            "focus": {
              "target": "preco-anuncio",
              "padding": 1.3
            }
          },
          "elements": [
            {
              "id": "preco-anuncio",
              "kind": "native",
              "asset": "price-tag",
              "x": 410,
              "y": 245,
              "width": 250,
              "height": 150,
              "color": "#FFCB69",
              "rotation": -5
            },
            {
              "id": "parcela-baixa",
              "kind": "text",
              "text": "PARCELA QUE CABE?",
              "x": 250,
              "y": 475,
              "width": 780,
              "height": 80,
              "fontSize": 40,
              "color": "#55E2C4"
            }
          ],
          "beats": [
            {
              "id": "preco-escala",
              "at": {
                "cue": 0,
                "wordIndex": 10
              },
              "action": "transform",
              "target": "preco-anuncio",
              "durationFrames": 10,
              "scale": 1.14,
              "rotation": 3
            },
            {
              "id": "som-da-etiqueta",
              "at": {
                "cue": 0,
                "wordIndex": 10
              },
              "action": "sound",
              "source": "media/soft-chime.wav",
              "gain": 0.65
            }
          ]
        },
        {
          "id": "oferta-detalhe",
          "start": {
            "cue": 1,
            "wordIndex": 4
          },
          "transitionIn": "cut",
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1,
            "focus": {
              "target": "contrato-oferta",
              "padding": 1.5
            }
          },
          "elements": [
            {
              "id": "contrato-oferta",
              "kind": "native",
              "asset": "document",
              "x": 500,
              "y": 235,
              "width": 220,
              "height": 275,
              "color": "#FFCB69"
            },
            {
              "id": "surpresa-oferta",
              "kind": "text",
              "text": "A CONTA COMPLETA AINDA NÃO APARECEU",
              "x": 210,
              "y": 555,
              "width": 860,
              "height": 52,
              "fontSize": 27,
              "color": "#F4F7FA"
            }
          ],
          "beats": [
            {
              "id": "oferta-documento-camera",
              "at": {
                "cue": 1,
                "wordIndex": 7
              },
              "action": "camera",
              "durationFrames": 18,
              "x": 610,
              "y": 340,
              "zoom": 1.2
            }
          ]
        }
      ]
    },
    {
      "id": "financiamento",
      "artDirection": {
        "backgroundColor": "#17131b",
        "glowColor": "#46304f",
        "primaryColor": "#f4b35b"
      },
      "narration": [
        "Quando existe financiamento, a parcela esconde juros, tarifas e seguros incluídos no contrato.",
        "Compare o custo efetivo total e some tudo que será pago até a última prestação."
      ],
      "shots": [
        {
          "id": "financiamento-papel",
          "start": {
            "cue": 0,
            "cueProgress": 0
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1
          },
          "elements": [
            {
              "id": "contrato-financiamento",
              "kind": "native",
              "asset": "document",
              "x": 360,
              "y": 225,
              "width": 230,
              "height": 285,
              "color": "#FFCB69",
              "rotation": -4
            },
            {
              "id": "moedas-financiamento",
              "kind": "native",
              "asset": "coins",
              "x": 700,
              "y": 360,
              "width": 240,
              "height": 190,
              "color": "#55E2C4"
            }
          ],
          "beats": [
            {
              "id": "financiamento-papel-aparece",
              "at": {
                "cue": 0,
                "wordIndex": 3
              },
              "action": "transform",
              "target": "contrato-financiamento",
              "durationFrames": 12,
              "rotation": 3,
              "scale": 1.06
            }
          ]
        },
        {
          "id": "financiamento-cet",
          "artDirection": {
            "primaryColor": "#ff5d72",
            "textColor": "#fff0db"
          },
          "start": {
            "cue": 0,
            "wordIndex": 6
          },
          "transitionIn": {
            "type": "slide",
            "direction": "left",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1.08,
            "focus": {
              "target": "cet-texto",
              "padding": 1.4
            }
          },
          "elements": [
            {
              "id": "cet-texto",
              "kind": "text",
              "text": "CET",
              "x": 410,
              "y": 220,
              "width": 460,
              "height": 110,
              "fontSize": 78,
              "color": "#55E2C4",
              "motion": "pop"
            },
            {
              "id": "cet-explicado",
              "kind": "text",
              "text": "CUSTO EFETIVO TOTAL",
              "x": 310,
              "y": 360,
              "width": 660,
              "height": 70,
              "fontSize": 34,
              "color": "#F4F7FA"
            }
          ],
          "beats": [
            {
              "id": "cet-destaque",
              "at": {
                "cue": 0,
                "wordIndex": 8
              },
              "action": "transform",
              "target": "cet-texto",
              "durationFrames": 8,
              "scale": 1.15
            }
          ]
        },
        {
          "id": "financiamento-total",
          "start": {
            "cue": 1,
            "wordIndex": 3
          },
          "transitionIn": {
            "type": "dissolve",
            "durationFrames": 10
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1
          },
          "elements": [
            {
              "id": "barras-financiamento",
              "kind": "native",
              "asset": "bars",
              "x": 445,
              "y": 245,
              "width": 390,
              "height": 285,
              "color": "#55E2C4"
            },
            {
              "id": "total-financiamento",
              "kind": "text",
              "text": "ENTRADA + JUROS + TARIFAS",
              "x": 210,
              "y": 565,
              "width": 860,
              "height": 50,
              "fontSize": 27,
              "color": "#FFCB69"
            }
          ],
          "beats": [
            {
              "id": "total-camera",
              "at": {
                "cue": 1,
                "wordIndex": 7
              },
              "action": "camera",
              "durationFrames": 14,
              "zoom": 1.14
            }
          ]
        }
      ]
    },
    {
      "id": "custo-de-uso",
      "narration": [
        "Depois de comprar, o carro continua cobrando: combustível, seguro, impostos, estacionamento e revisões periódicas.",
        "Uma manutenção atrasada ou uma peça inesperada também pode transformar economia em dívida."
      ],
      "shots": [
        {
          "id": "uso-carro",
          "start": {
            "cue": 0,
            "cueProgress": 0
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1,
            "focus": {
              "target": "carro-em-uso",
              "padding": 1.8
            }
          },
          "elements": [
            {
              "id": "carro-em-uso",
              "kind": "native",
              "asset": "car",
              "x": 355,
              "y": 350,
              "width": 560,
              "height": 255,
              "color": "#FFCB69",
              "motion": "slide-right",
              "travelX": 40
            },
            {
              "id": "personagem-uso",
              "kind": "native",
              "asset": "person",
              "x": 930,
              "y": 260,
              "width": 140,
              "height": 280,
              "pose": "worried",
              "color": "#55E2C4"
            }
          ],
          "beats": [
            {
              "id": "uso-camera",
              "at": {
                "cue": 0,
                "wordIndex": 4
              },
              "action": "camera",
              "durationFrames": 12,
              "x": 670,
              "y": 355,
              "zoom": 1.12
            }
          ]
        },
        {
          "id": "uso-lista",
          "start": {
            "cue": 0,
            "wordIndex": 7
          },
          "transitionIn": {
            "type": "zoom",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1.05
          },
          "elements": [
            {
              "id": "lista-custos",
              "kind": "text",
              "text": "COMBUSTÍVEL · SEGURO\nIMPOSTOS · REVISÕES",
              "x": 190,
              "y": 250,
              "width": 900,
              "height": 180,
              "fontSize": 39,
              "color": "#F4F7FA",
              "motion": "rise"
            },
            {
              "id": "moedas-custos",
              "kind": "native",
              "asset": "coins",
              "x": 520,
              "y": 455,
              "width": 170,
              "height": 135,
              "color": "#FFCB69"
            }
          ],
          "beats": [
            {
              "id": "lista-custos-muda",
              "at": {
                "cue": 0,
                "wordIndex": 9
              },
              "action": "transform",
              "target": "lista-custos",
              "durationFrames": 10,
              "y": 215,
              "scale": 1.06
            }
          ]
        },
        {
          "id": "uso-manutencao",
          "start": {
            "cue": 1,
            "wordIndex": 3
          },
          "transitionIn": {
            "type": "dissolve",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1
          },
          "elements": [
            {
              "id": "documento-manutencao",
              "kind": "native",
              "asset": "document",
              "x": 505,
              "y": 245,
              "width": 210,
              "height": 270,
              "color": "#FFCB69"
            },
            {
              "id": "aviso-manutencao",
              "kind": "text",
              "text": "IMPREVISTO TAMBÉM ENTRA NA CONTA",
              "x": 250,
              "y": 555,
              "width": 780,
              "height": 50,
              "fontSize": 28,
              "color": "#55E2C4"
            }
          ],
          "beats": [
            {
              "id": "aviso-manutencao-revela",
              "at": {
                "cue": 1,
                "wordIndex": 6
              },
              "action": "show",
              "target": "aviso-manutencao"
            }
          ]
        }
      ]
    },
    {
      "id": "carro-usado",
      "narration": [
        "Carro usado exige mais do que olhar a lataria; histórico, quilometragem e inspeção mecânica contam.",
        "Antes de fechar negócio, peça documentos, compare avaliações e reserve dinheiro para reparos."
      ],
      "shots": [
        {
          "id": "usado-inspecao",
          "start": {
            "cue": 0,
            "cueProgress": 0
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1
          },
          "elements": [
            {
              "id": "carro-inspecao",
              "kind": "native",
              "asset": "car",
              "x": 345,
              "y": 355,
              "width": 550,
              "height": 250,
              "color": "#55E2C4"
            },
            {
              "id": "inspector-usado",
              "kind": "native",
              "asset": "person",
              "x": 880,
              "y": 265,
              "width": 145,
              "height": 290,
              "pose": "point",
              "color": "#FFCB69"
            }
          ],
          "beats": [
            {
              "id": "inspecao-aponta",
              "at": {
                "cue": 0,
                "wordIndex": 3
              },
              "action": "transform",
              "target": "inspector-usado",
              "durationFrames": 8,
              "rotation": -4
            }
          ]
        },
        {
          "id": "usado-historico",
          "start": {
            "cue": 0,
            "wordIndex": 7
          },
          "transitionIn": {
            "type": "slide",
            "direction": "right",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1.1,
            "focus": {
              "target": "historico-carro",
              "padding": 1.5
            }
          },
          "elements": [
            {
              "id": "historico-carro",
              "kind": "native",
              "asset": "document",
              "x": 465,
              "y": 235,
              "width": 220,
              "height": 275,
              "color": "#FFCB69"
            },
            {
              "id": "historico-titulo",
              "kind": "text",
              "text": "HISTÓRICO · KM · MECÂNICA",
              "x": 220,
              "y": 545,
              "width": 840,
              "height": 55,
              "fontSize": 29,
              "color": "#F4F7FA"
            }
          ],
          "beats": [
            {
              "id": "historico-camera",
              "at": {
                "cue": 0,
                "wordIndex": 10
              },
              "action": "camera",
              "durationFrames": 14,
              "zoom": 1.22
            }
          ]
        },
        {
          "id": "usado-reserva",
          "start": {
            "cue": 1,
            "wordIndex": 3
          },
          "transitionIn": {
            "type": "dissolve",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1
          },
          "elements": [
            {
              "id": "moedas-reparo",
              "kind": "native",
              "asset": "coins",
              "x": 490,
              "y": 300,
              "width": 240,
              "height": 190,
              "color": "#55E2C4"
            },
            {
              "id": "reserva-reparo",
              "kind": "text",
              "text": "RESERVE PARA REPAROS",
              "x": 300,
              "y": 545,
              "width": 680,
              "height": 55,
              "fontSize": 32,
              "color": "#FFCB69"
            }
          ],
          "beats": [
            {
              "id": "reserva-revela",
              "at": {
                "cue": 1,
                "wordIndex": 7
              },
              "action": "show",
              "target": "reserva-reparo"
            }
          ]
        }
      ]
    },
    {
      "id": "orcamento-real",
      "narration": [
        "Faça uma conta mensal realista, incluindo despesas fixas, viagens e uma margem para emergências.",
        "Se a conta só fecha ignorando custos, talvez o carro esteja acima do seu orçamento."
      ],
      "shots": [
        {
          "id": "orcamento-planilha",
          "start": {
            "cue": 0,
            "cueProgress": 0
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1
          },
          "elements": [
            {
              "id": "planilha-orcamento",
              "kind": "native",
              "asset": "document",
              "x": 500,
              "y": 225,
              "width": 220,
              "height": 280,
              "color": "#F4F7FA"
            },
            {
              "id": "titulo-orcamento",
              "kind": "text",
              "text": "A CONTA DO MÊS",
              "x": 250,
              "y": 125,
              "width": 780,
              "height": 80,
              "fontSize": 49,
              "color": "#55E2C4"
            }
          ],
          "beats": [
            {
              "id": "orcamento-titulo-revela",
              "at": {
                "cue": 0,
                "wordIndex": 4
              },
              "action": "show",
              "target": "titulo-orcamento"
            }
          ]
        },
        {
          "id": "orcamento-barras",
          "start": {
            "cue": 0,
            "wordIndex": 7
          },
          "transitionIn": {
            "type": "dissolve",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1.08,
            "focus": {
              "target": "barras-orcamento",
              "padding": 1.2
            }
          },
          "elements": [
            {
              "id": "barras-orcamento",
              "kind": "native",
              "asset": "bars",
              "x": 420,
              "y": 240,
              "width": 420,
              "height": 300,
              "color": "#55E2C4"
            },
            {
              "id": "reserva-margem",
              "kind": "text",
              "text": "+ MARGEM PARA IMPREVISTOS",
              "x": 260,
              "y": 560,
              "width": 760,
              "height": 48,
              "fontSize": 25,
              "color": "#FFCB69"
            }
          ],
          "beats": [
            {
              "id": "barras-crescem",
              "at": {
                "cue": 0,
                "wordIndex": 10
              },
              "action": "transform",
              "target": "barras-orcamento",
              "durationFrames": 12,
              "scale": 1.08
            }
          ]
        },
        {
          "id": "orcamento-alerta",
          "start": {
            "cue": 1,
            "wordIndex": 4
          },
          "transitionIn": {
            "type": "zoom",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1
          },
          "elements": [
            {
              "id": "alerta-orcamento",
              "kind": "text",
              "text": "SE A CONTA NÃO FECHA, PARE E REAVALIE",
              "x": 180,
              "y": 280,
              "width": 920,
              "height": 120,
              "fontSize": 39,
              "color": "#FFCB69",
              "motion": "pop"
            },
            {
              "id": "personagem-orcamento",
              "kind": "native",
              "asset": "person",
              "x": 550,
              "y": 430,
              "width": 135,
              "height": 260,
              "pose": "worried",
              "color": "#55E2C4"
            }
          ],
          "beats": [
            {
              "id": "orcamento-alerta-camera",
              "at": {
                "cue": 1,
                "wordIndex": 8
              },
              "action": "camera",
              "durationFrames": 14,
              "zoom": 1.12
            }
          ]
        }
      ]
    },
    {
      "id": "decisao-consciente",
      "narration": [
        "Um bom negócio não é simplesmente pagar menos no anúncio ou escolher a menor parcela.",
        "É escolher com informação, entender o custo completo e manter espaço para outros objetivos."
      ],
      "shots": [
        {
          "id": "decisao-anuncio",
          "start": {
            "cue": 0,
            "cueProgress": 0
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1
          },
          "elements": [
            {
              "id": "carro-decisao",
              "kind": "native",
              "asset": "car",
              "x": 360,
              "y": 355,
              "width": 540,
              "height": 245,
              "color": "#55E2C4"
            },
            {
              "id": "preco-decisao",
              "kind": "native",
              "asset": "price-tag",
              "x": 750,
              "y": 245,
              "width": 210,
              "height": 125,
              "color": "#FFCB69",
              "rotation": 5
            }
          ],
          "beats": [
            {
              "id": "decisao-preco-esconde",
              "at": {
                "cue": 0,
                "wordIndex": 5
              },
              "action": "hide",
              "target": "preco-decisao"
            }
          ]
        },
        {
          "id": "decisao-clareza",
          "start": {
            "cue": 0,
            "wordIndex": 7
          },
          "transitionIn": {
            "type": "dissolve",
            "durationFrames": 12
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1.08
          },
          "elements": [
            {
              "id": "decisao-documento",
              "kind": "native",
              "asset": "document",
              "x": 480,
              "y": 230,
              "width": 220,
              "height": 275,
              "color": "#F4F7FA"
            },
            {
              "id": "decisao-clara",
              "kind": "text",
              "text": "COMPARE O CUSTO TOTAL",
              "x": 260,
              "y": 540,
              "width": 760,
              "height": 60,
              "fontSize": 34,
              "color": "#55E2C4"
            }
          ],
          "beats": [
            {
              "id": "decisao-texto",
              "at": {
                "cue": 0,
                "wordIndex": 10
              },
              "action": "transform",
              "target": "decisao-clara",
              "durationFrames": 10,
              "scale": 1.08
            }
          ]
        },
        {
          "id": "decisao-final",
          "start": {
            "cue": 1,
            "wordIndex": 3
          },
          "transitionIn": {
            "type": "fade",
            "durationFrames": 18
          },
          "camera": {
            "x": 640,
            "y": 360,
            "zoom": 1,
            "focus": {
              "target": "personagem-final",
              "padding": 1.5
            }
          },
          "elements": [
            {
              "id": "personagem-final",
              "kind": "native",
              "asset": "person",
              "x": 520,
              "y": 230,
              "width": 165,
              "height": 320,
              "pose": "wave",
              "color": "#55E2C4"
            },
            {
              "id": "mensagem-final",
              "kind": "text",
              "text": "ANTES DA CHAVE, FAÇA A CONTA INTEIRA",
              "x": 155,
              "y": 100,
              "width": 970,
              "height": 80,
              "fontSize": 42,
              "color": "#F4F7FA"
            }
          ],
          "beats": [
            {
              "id": "final-camera",
              "at": {
                "cue": 1,
                "wordIndex": 8
              },
              "action": "camera",
              "durationFrames": 18,
              "zoom": 1.14
            }
          ]
        }
      ]
    }
  ]
}
```
<!-- STUDIO-DARK-VALIDATED-EXAMPLE-END -->

## Composição, grupos e câmera

Não há cabeçalho, título de cena ou cenário forçado em v2. `title`, `kicker` e `background` são opcionais por cena. Sem fundo nativo, o compositor mantém seu gradiente escuro base. Posicione texto e objetos onde fizer sentido; legendas automáticas ocupam a faixa inferior.

`group` exige `children` e pode aninhar até 8 níveis. Coordenadas dos filhos são locais ao canto superior esquerdo do grupo pai. A ordem da lista determina a ordem de desenho. Um grupo pode ter `x`, `y`, `width`, `height`, `rotation`, `scale`, `opacity`, `cue`, `offsetSeconds` e `span` como outro elemento.

`keyframes` em elementos são estados de `x`, `y`, `rotation`, `scale` e `opacity`. Cada chave usa `cue` e `offsetSeconds`; propriedades omitidas mantêm o valor inicial daquele elemento. Opcionalmente, `easing` na **chave de destino** escolhe a interpolação até ela: `linear` (padrão), `ease-in`, `ease-out` ou `ease-in-out`. A primeira chave não tem trecho anterior. As chaves devem cair dentro do trecho de vida do elemento. Evite combinar `keyframes` com `motion`, `travelX`, `travelY` ou `spinDegrees` no mesmo item.

`camera` é uma lista de keyframes na cena com `cue`, `x`, `y`, `zoom` e `rotation`; `offsetSeconds` e `easing` são opcionais. A curva tem a mesma semântica da chave de destino usada nos elementos. O centro inicial é 640×360, zoom 1 e rotação 0. A câmera atravessa os limites entre cenas e mantém o último estado até a próxima chave. Zoom válido: 0.1 a 10. `cue` começa em zero em cada cena.

Para sincronizar, informe `cue` e opcionalmente `offsetSeconds` de 0 a 10. O compilador mede o áudio real. `span` aceita `scene` (padrão) ou `utterance`. Não use timestamps inventados.

## Elementos

`kind`: `group`, `library`, `native`, `text`, `shape`, `image`, `svg` ou `video` (vídeo requer receita v3). `native` usa o campo `asset`; texto usa `text`; mídia direta usa `source`, caminho local absoluto ou relativo ao JSON. Arquivos remotos e URLs não são baixados. PNG com transparência e SVG compatível com Qt são aceitos. Vídeos locais MP4, MOV, MKV, WebM, AVI e M4V são decodificados em quadros pela mesma composição usada na prévia e exportação. Eles mantêm proporção e preenchem a caixa com corte central; o decodificador reutiliza um cache limitado a 64 MiB e faz busca direta em saltos longos. O ajuste do seek respeita o primeiro quadro real depois do trim, mantendo a mesma cadência da decodificação sequencial. Nenhum campo extra de receita é necessário.

Um vídeo pode incluir `video: {"sourceInSeconds":1.25,"durationFrames":120,"speed":1.5,"crop":{"x":0.1,"y":0.1,"width":0.8,"height":0.8}}`. `sourceInSeconds` começa a leitura nessa posição da fonte (0–3600 s); `durationFrames` define a duração de saída em quadros da timeline (1–18000), incluindo o efeito da velocidade; `speed` controla a velocidade (0.25–4, padrão 1); o recorte normalizado usa origem `x`,`y` e tamanho `width`,`height` entre 0 e 1, dentro da imagem. Se `durationFrames` for omitido, o vídeo ocupa o restante do intervalo do shot/cena. O inspector também permite editar a duração do clipe diretamente na timeline. Sem `video`, o comportamento antigo de início zero, velocidade 1 e enquadramento central permanece. Projetos com trim/fonte, velocidade ou crop personalizado usam schema 10; versões anteriores continuam abrindo.

Propriedades: `x`, `y`, `width`, `height`, `cueProgress` (alternativo a `offsetSeconds`), `color` (#RRGGBB), `secondaryColor`, `fontSize` (8–300), `opacity` (0–1), `rotation`, `scale` (0.01–10), `travelX`, `travelY`, `spinDegrees`, `motion`, `motionFrames` (1–300), `exitFrames` (0–300), `pose`, `cue`, `offsetSeconds`, `span`, `keyframes`, `children`, `assetId`.

Movimentos: `none`, `fade`, `rise`, `slide-left`, `slide-right`, `pop`. Poses: `neutral`, `point`, `wave`, `worried`. Roupas/personagem usam `color`; `secondaryColor` controla a pele. Partes de personagem ainda não são objetos articulados.

Biblioteca nativa disponível: car, person, house, building, document, computer, phone, coins, price-tag, bars, street, office, showroom e studio. Barras são ilustrativas e não representam números de pesquisa.

## Biblioteca pessoal desta cópia do guia

Esta cópia não recebeu o catálogo local. Salve o guia pelo botão no app para incluir sua biblioteca pessoal.

Use um item deste manifesto com `{"kind":"library","assetId":"ID-DO-ITEM","cue":0}`. Itens apagados não aparecem. O aplicativo resolve IDs na biblioteca local e informa se o item foi removido ou não existe. Não invente IDs nem suponha que outra máquina tenha os mesmos arquivos. O manifesto descreve nomes, categorias e dimensões; ele não inspeciona visualmente a imagem. Se a descrição não bastar para decidir, pergunte ao usuário. Itens image/SVG também podem ser referenciados por `source` se o usuário fornecer um caminho.

## Presets compostos na biblioteca pessoal

O app agora permite salvar e reutilizar um **grupo raiz** com filhos, subgrupos, animações e mídias copiadas para a biblioteca. O guia exportado pelo aplicativo identifica esses presets; o guia do repositório não contém IDs pessoais.

Em receitas v2, `{"id":"painel1","kind":"library","assetId":"ID-DO-PRESET","cue":0}` também instancia grupos inteiros. Os filhos mantêm posições locais, duração e keyframes em frames relativos ao cue escolhido (30 FPS); o importador cria novos IDs e preserva a hierarquia. A duração efetiva é limitada pela cena/trecho medido: uma chave que cair depois do fim provoca erro, não um corte silencioso. Se a animação precisa ser ajustada à narração, descreva o grupo diretamente em `children` com keyframes por cue.

Propriedades visuais passadas à referência alteram a raiz; keyframes salvos continuam controlando a animação dessa raiz. Não combine keyframes explícitos com o mesmo preset animado. Se o item do manifesto for uma imagem, seu nome não confirma o conteúdo visual: solicite descrição quando necessário. IDs dependem da instalação e devem ser obtidos exportando o guia atualizado no próprio app.

## Catálogo nativo do aplicativo

- `car` — Carro (objetos), 460×210.
- `person` — Personagem (personagens), 180×350.
- `house` — Casa (objetos), 240×210.
- `building` — Prédio (objetos), 160×300.
- `document` — Documento (objetos), 200×250.
- `computer` — Computador (objetos), 280×210.
- `phone` — Celular (objetos), 120×220.
- `coins` — Moedas (objetos), 250×200.
- `price-tag` — Etiqueta (objetos), 250×150.
- `bars` — Barras ilustrativas (dados), 320×240.
- `street` — Rua (cenários), 1280×720.
- `office` — Escritório (cenários), 1280×720.
- `showroom` — Concessionária (cenários), 1280×720.
- `studio` — Estúdio (cenários), 1280×720.


`musicGain`: 0 a 0.08; zero desliga a trilha procedural. `effectsGain`: 0 a 0.15; zero desliga sinais de troca de cena. `sources` pode registrar título e URL HTTPS; é referência editorial, não verificação factual. Revise fatos, fontes e direitos de mídia antes de publicar.

A receita v1 antiga continua sendo lida, com cabeçalho/cenário/layout legado. Receitas v2 mantêm grupos, presets, câmera e keyframes; v3 acrescenta shots, beats, direção de arte por escopo, camadas de vídeo, fades, automação de áudio e animação de legenda por palavra. Campos desconhecidos, IDs duplicados, itens pessoais inativos, arquivos ausentes e limites fora da faixa são rejeitados com mensagem. Projetos v1–v13 continuam abrindo; camadas de vídeo usam schema 5, fades de pista não nulos usam schema 6, legendas com destaque sincronizado por palavra usam schema 7, automação de ganho usa schema 8, efeitos de Visual Beat usam schema 9, controles de fonte de vídeo usam schema 10, estilos por cena/shot usam schema 11, presets de animação por palavra usam schema 12 e trim/velocidade da fonte de áudio usam schema 13.

## Checklist de limites e publicação

O app não baixa mídias da Internet nem gera automaticamente imagens ou vídeos a partir de um prompt. Caminhos relativos de `source` são relativos ao arquivo `.production.json`, em UTF-8. `sources` com URLs HTTPS guarda **referências editoriais**, não verifica fatos nem cria citações visuais. Confira direitos de uso e dados antes de publicar.

Vídeo local como camada com trim de entrada, velocidade, recorte manual, cache de quadros limitado e seek temporal, áudio de vídeo local em pista editável com trim/velocidade, direção de arte global/cena/shot, sincronização por palavra em shots/beats/elementos/keyframes e destaque palavra a palavra nas legendas, seleção múltipla com movimento conjunto na timeline, biblioteca vetorial, áudio em várias pistas, ducking, fades, automação editável graficamente, efeitos sonoros disparados por Visual Beat e telas 1280×720, 1920×1080 ou 1080×1920 estão implementados. Continuam pendentes documentos/PDF com regiões, conectores ancorados, trajetórias Bézier personalizadas, presets visuais nomeados e outras ferramentas avançadas de timeline. Não descreva essas funções pendentes como disponíveis no JSON.

## Capacidades em JSON

```json
{
    "audio": [
        "windows-pt-BR-tts",
        "windows-sapi-word-boundaries",
        "procedural-bed",
        "scene-chime",
        "multi-track-wav-mp3-aac-ogg-flac-wma",
        "voice-sidechain-ducking"
    ],
    "catalogVersion": 1,
    "format": "studio-dark-capabilities",
    "fps": 30,
    "height": 720,
    "maxAudioTracks": 32,
    "maxNarrationCharactersPerCue": 150,
    "maxNodes": 200,
    "maxScenes": 20,
    "maxShots": 256,
    "maxShotsPerScene": 64,
    "maxTransitionDurationFrames": 300,
    "maxVisualBeats": 4096,
    "media": [
        "png",
        "jpeg",
        "webp",
        "bmp",
        "svg",
        "mp4",
        "mov",
        "mkv",
        "webm",
        "avi",
        "m4v"
    ],
    "motions": [
        "none",
        "fade",
        "rise",
        "slide-left",
        "slide-right",
        "pop"
    ],
    "nativeAssets": [
        {
            "category": "objetos",
            "height": 210,
            "id": "car",
            "name": "Carro",
            "width": 460
        },
        {
            "category": "personagens",
            "height": 350,
            "id": "person",
            "name": "Personagem",
            "width": 180
        },
        {
            "category": "objetos",
            "height": 210,
            "id": "house",
            "name": "Casa",
            "width": 240
        },
        {
            "category": "objetos",
            "height": 300,
            "id": "building",
            "name": "Prédio",
            "width": 160
        },
        {
            "category": "objetos",
            "height": 250,
            "id": "document",
            "name": "Documento",
            "width": 200
        },
        {
            "category": "objetos",
            "height": 210,
            "id": "computer",
            "name": "Computador",
            "width": 280
        },
        {
            "category": "objetos",
            "height": 220,
            "id": "phone",
            "name": "Celular",
            "width": 120
        },
        {
            "category": "objetos",
            "height": 200,
            "id": "coins",
            "name": "Moedas",
            "width": 250
        },
        {
            "category": "objetos",
            "height": 150,
            "id": "price-tag",
            "name": "Etiqueta",
            "width": 250
        },
        {
            "category": "dados",
            "height": 240,
            "id": "bars",
            "name": "Barras ilustrativas",
            "width": 320
        },
        {
            "category": "cenários",
            "height": 720,
            "id": "street",
            "name": "Rua",
            "width": 1280
        },
        {
            "category": "cenários",
            "height": 720,
            "id": "office",
            "name": "Escritório",
            "width": 1280
        },
        {
            "category": "cenários",
            "height": 720,
            "id": "showroom",
            "name": "Concessionária",
            "width": 1280
        },
        {
            "category": "cenários",
            "height": 720,
            "id": "studio",
            "name": "Estúdio",
            "width": 1280
        }
    ],
    "poses": [
        "neutral",
        "point",
        "wave",
        "worried"
    ],
    "productionCapabilities": [
        "free-scene-layout",
        "nested-groups",
        "transform-keyframes-linear",
        "camera-2d-keyframes",
        "keyframe-easing-curves",
        "personal-library-assets",
        "reusable-group-presets",
        "optional-visual-captions",
        "fixed-duration-timing",
        "cue-progress-sync",
        "pitch-preserving-atempo",
        "independent-shots",
        "visual-beats-show-hide-transform-camera-sound",
        "shot-cut-camera",
        "narration-content-shot-rhythm",
        "shot-transitions-fade-dissolve-slide-zoom",
        "auto-reframe-by-shot-target",
        "continuous-shot-focus-follow",
        "intensity-driven-default-motion",
        "reusable-camera-keyframe-presets",
        "global-art-direction",
        "scene-shot-art-direction",
        "word-aligned-visual-beats",
        "embedded-video-audio-with-source-trim-and-speed",
        "word-aligned-elements-keyframes",
        "multi-track-audio",
        "visual-beat-sound-effects",
        "voice-sidechain-ducking",
        "audio-track-fades",
        "audio-gain-automation-easing",
        "video-layer-broll",
        "video-source-trim",
        "video-speed-control",
        "video-normalized-crop",
        "landscape-and-portrait-canvas-presets",
        "word-timed-karaoke-captions",
        "word-animated-karaoke-captions"
    ],
    "productionVersion": 3,
    "projectCapabilities": [
        "group-hierarchy",
        "transform-keyframes-linear",
        "camera-2d-keyframes",
        "keyframe-easing-curves",
        "shot-ownership",
        "visual-beat-markers",
        "audio-tracks",
        "visual-beat-linked-audio-tracks",
        "video-layers",
        "video-trim-speed-crop",
        "scene-art-direction",
        "shot-art-direction",
        "audio-track-fades",
        "audio-gain-automation",
        "word-timed-caption-highlights",
        "word-animated-caption-presets"
    ],
    "projectVersion": 13,
    "shotTransitionTypes": [
        "cut",
        "fade",
        "dissolve",
        "slide",
        "zoom"
    ],
    "supportedCanvases": [
        {
            "height": 720,
            "name": "Paisagem HD",
            "width": 1280
        },
        {
            "height": 1080,
            "name": "Paisagem Full HD",
            "width": 1920
        },
        {
            "height": 1920,
            "name": "Retrato",
            "width": 1080
        }
    ],
    "width": 1280
}
```
