#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QStringList>
#include <QSvgRenderer>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>

namespace studio {
QJsonObject capabilities() {
    QJsonArray items, motions, poses;
    for (const auto &asset : nativeLibrary())
        items.append(QJsonObject{{"id", QString::fromStdString(asset.id)},
                                 {"name", QString::fromStdString(asset.name)},
                                 {"category", QString::fromStdString(asset.category)},
                                 {"width", asset.width},
                                 {"height", asset.height}});
    for (const auto &motion : motionNames())
        motions.append(QString::fromStdString(motion));
    for (const auto &pose : poseNames())
        poses.append(QString::fromStdString(pose));
    return {{"format", "studio-dark-capabilities"},
            {"catalogVersion", 1},
            {"productionVersion", 3},
            {"projectVersion", 8},
            {"productionCapabilities",
             QJsonArray{"free-scene-layout", "nested-groups", "transform-keyframes-linear",
                        "camera-2d-keyframes", "keyframe-easing-curves", "personal-library-assets",
                        "reusable-group-presets", "optional-visual-captions", "fixed-duration-timing",
                        "cue-progress-sync", "pitch-preserving-atempo", "independent-shots",
                        "visual-beats-show-hide-transform-camera", "shot-cut-camera",
                        "shot-transitions-fade-dissolve-slide-zoom", "auto-reframe-by-shot-target",
                        "continuous-shot-focus-follow", "intensity-driven-default-motion",
                        "global-art-direction", "word-aligned-visual-beats", "word-aligned-elements-keyframes",
                        "multi-track-audio",
                        "voice-sidechain-ducking", "audio-track-fades", "audio-gain-automation-easing",
                        "video-layer-broll",
                        "word-timed-karaoke-captions"}},
            {"projectCapabilities",
             QJsonArray{"group-hierarchy", "transform-keyframes-linear", "camera-2d-keyframes",
                        "keyframe-easing-curves", "shot-ownership", "visual-beat-markers", "audio-tracks",
                        "video-layers", "audio-track-fades", "audio-gain-automation",
                        "word-timed-caption-highlights"}},
            {"width", 1280},
            {"height", 720},
            {"fps", 30},
            {"nativeAssets", items},
            {"motions", motions},
            {"poses", poses},
            {"media", QJsonArray{"png", "jpeg", "webp", "bmp", "svg", "mp4", "mov", "mkv", "webm",
                                 "avi", "m4v"}},
            {"maxScenes", 20},
            {"maxShotsPerScene", 64},
            {"maxShots", 256},
            {"maxVisualBeats", 4096},
            {"maxAudioTracks", 32},
            {"shotTransitionTypes", QJsonArray{"cut", "fade", "dissolve", "slide", "zoom"}},
            {"maxTransitionDurationFrames", 300},
            {"maxNodes", 200},
            {"maxNarrationCharactersPerCue", 150},
            {"audio", QJsonArray{"windows-pt-BR-tts", "windows-sapi-word-boundaries",
                                 "procedural-bed", "scene-chime", "multi-track-wav-mp3-aac-ogg-flac-wma",
                                 "voice-sidechain-ducking"}}};
}
QString aiGuide(const QJsonObject &library) {
    QString guide = QString::fromUtf8(R"GUIDE(# Studio Dark — guia para dirigir um vídeo com IA

Catálogo 1 · Receita 3 · Projeto 8 · Native Lab 0.4

Você é o diretor audiovisual. Transforme o roteiro em uma produção editável. Preserve o texto do usuário, salvo pedido explícito de reescrita. Escolha enquadramentos e composições livres usando somente os recursos abaixo. Entregue JSON puro, sem cercas Markdown.

## Fluxo

O usuário escreve o roteiro fora do aplicativo, fornece este guia à IA e salva a resposta em `.production.json`. **Importar produção IA** gera voz local, mede cada fala, resolve os cues, monta o projeto e sincroniza o áudio. A pessoa pode editar cada grupo e elemento no editor e exportar o vídeo. Não invente duração nem timestamps da fala.

Receitas v1 e v2 continuam aceitas por compatibilidade. Para novas produções com cortes dentro da mesma fala, use v3.

## Como planejar a receita

- Responda **somente com JSON válido**, sem comentários, cercas Markdown nem texto de apresentação. Use `format: "studio-dark-production"` e `catalogVersion: 1`; escolha `version: 3` para shots/Visual Beats ou `version: 2` para a composição de cena tradicional.
- Preserve o roteiro recebido, salvo instrução de reescrever. Divida cada cena em trechos faláveis, de no máximo 150 caracteres, e use `cue` como índice (a partir de zero) na narração da **mesma cena**.
- `offsetSeconds` varia de 0 a 10; `cueProgress` usa de 0 a 1 para acompanhar a fala final. Sem duração fixa, a duração natural depende da voz pt-BR e só é conhecida depois de sintetizar o WAV.
- Monte a narrativa com composição livre, `group`, `shape`, `text`, `native`, `image`, `svg`, `video`, biblioteca pessoal, câmera e keyframes documentados. Reserve espaço para as legendas se `captions` for `true`.
- Conte no máximo 200 nós, incluindo filhos, legendas automáticas, título/kicker e cenário. Evite elementos ilegíveis em 1280×720; respeite o limite de oito níveis de grupo.
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

`captions` controla as legendas visuais automáticas (padrão `true`). O arquivo SRT ainda é criado mesmo quando `captions` é `false`. Quando a voz Windows pt-BR emite alinhamento real, a legenda visual destaca a palavra falada com `artDirection.primaryColor` (ou a cor primária padrão). O projeto guarda os intervalos em quadros no schema 7; isso não exige um campo adicional na receita. Sem alinhamento real, a legenda continua estática. O SRT não contém cores.

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

Uma cena narrativa pode conter vários shots sem repetir nem interromper a narração. A versão 3 aceita as configurações globais da v2 e substitui `elements` e `camera` da cena por `shots`. Cada cena v3 exige de 1 a 64 shots; a produção aceita até 256 shots e 4096 Visual Beats. O primeiro shot começa junto ao início da cena. O início do shot seguinte encerra o anterior; o último termina com a cena. Assim, os planos cobrem a cena sem lacunas ou sobreposições.

Cada cena v3 também pode declarar `audioTracks` (até 32 no projeto). Uma pista usa `id`, `source` (arquivo de áudio local relativo ao JSON ou absoluto), `start` (a mesma referência a cue/progresso/wordIndex), `gain` (0–4), `duckUnderVoice` e `duckAmount` (0–1). `name` é opcional. `fadeInFrames` e `fadeOutFrames` são opcionais (0–1800) e aplicam fades lineares ao começo/fim da pista; a soma deve caber no intervalo disponível. `gainAutomation` é opcional e aceita até 128 pontos `{"frame":0,"value":0.5,"easing":"linear"}`; o quadro é relativo ao início da pista, `value` multiplica o ganho base (0–4), e `easing` aceita `linear`, `ease-in`, `ease-out` ou `ease-in-out`. Os pontos devem estar em ordem crescente. Antes do primeiro e depois do último ponto, mantém-se o nível mais próximo. A pista começa no ponto indicado e segue até o fim da cena. `duckUnderVoice:true` usa o WAV de voz isolado como referência para reduzir a pista durante a fala; sem stem dedicado, usa `audioSource`. Quando a referência é WAV PCM mono a 24 kHz, prévia e exportação usam a mesma envoltória RMS por quadro. Outros formatos usam o sidechain FFmpeg na exportação; para sincronização idêntica à prévia, forneça um WAV compatível. Pistas ficam editáveis no projeto; a timeline permite adicionar pontos com easing no playhead, substituir um ponto na mesma posição e limpar a automação.

Cada shot exige `id`, `start`, `camera` e `elements`; `name` e `transitionIn` são opcionais. IDs são únicos em toda a receita. `start` usa `cue` e um único campo entre `cueProgress` (0–1), `progress` (alias compatível), `offsetSeconds` (0–10) ou `wordIndex` (índice da palavra, começando em zero). `wordIndex` usa os eventos de progresso emitidos pela voz Windows pt-BR e não pode ser combinado com outros deslocamentos. O tempo é resolvido sobre o áudio medido. `transitionIn` pode ser a string legada `cut`, um objeto `{"type":"cut"}` ou um objeto de transição animada. Um `durationFrames` opcional junto a `type:"cut"` é aceito por compatibilidade e normalizado para corte imediato. O primeiro shot da cena sempre começa sem transição. Transições animadas sobrepõem visualmente o último quadro do shot anterior e o novo shot, sem alterar a duração da narração nem o tempo total da cena.

Exemplos de transição: `{"type":"fade","durationFrames":18}`, `{"type":"dissolve","durationFrames":12}`, `{"type":"slide","direction":"left","durationFrames":16}` ou `{"type":"zoom","durationFrames":12}`. `durationFrames` aceita de 2 a 300 quadros e precisa caber no shot de entrada. `slide` aceita direção `left`, `right`, `up` ou `down`. `fade` atravessa preto; `dissolve` mistura as imagens; `slide` move o novo shot; `zoom` aproxima o novo shot ao entrar. Prévia e exportação usam o mesmo compositor. A timeline destaca a faixa de transição.

`camera` aceita `x`, `y`, `zoom` e `rotation`; quando omitidos, o centro é 640×360, o zoom é 1 e a rotação é 0.

Para enquadrar automaticamente um elemento ou grupo, adicione `"focus":{"target":"ID-DO-ELEMENTO","padding":1.25}` dentro de `camera`. O alvo deve pertencer ao mesmo shot. O aplicativo transforma os limites locais através de todos os grupos ancestrais, centraliza a câmera nesse retângulo e calcula o zoom para mantê-lo dentro do quadro com a margem solicitada. `padding` aceita 1–4 e assume 1.25. Quando `focus` está presente, ele substitui o centro e o zoom explícitos; a rotação continua sendo usada. Com `"follow":true`, o aplicativo recalcula o enquadramento para cada quadro do shot e grava a trajetória como keyframes editáveis. `follow` assume false. Não combine esse modo com Visual Beats de ação `camera` no mesmo shot.

Elementos declarados dentro do shot pertencem apenas a ele e viram faixas visuais editáveis no projeto. Sem referência temporal explícita, começam no início do shot. Os elementos e grupos continuam usando os tipos e propriedades documentados abaixo.

`editing.intensity` é opcional e aceita inteiro de 0 a 100. Ele só preenche `motion`, `motionFrames` e `exitFrames` omitidos nos elementos: 0 desliga movimentos automáticos, 1–30 usa entradas discretas com fade, 31–70 usa entradas rise e 71–100 usa pop mais rápido. Campos explicitamente declarados pela IA não são substituídos. Sem o objeto `editing`, receitas v3 mantêm os defaults anteriores; receitas v1/v2 não aceitam essa opção. Intensidade ainda não cria cortes nem beats sem direção narrativa.

`artDirection` é opcional na raiz da receita v3 e define o estilo global: `backgroundColor`, `glowColor`, `primaryColor`, `secondaryColor`, `textColor`, `cardColor` e `captionBackgroundColor` usam `#RRGGBB`; `fontFamily` aceita nome de fonte instalado (até 120 caracteres); `titleSize` e `bodySize` aceitam 8–160 px; `captionSize` aceita 8–120 px; `captionPosition` aceita `top` ou `bottom`; `safeMargin` aceita 0–300 px. Cores/tamanho/fonte são aplicados como defaults a objetos e texto, e elementos com `color`, `secondaryColor` ou `fontSize` explícitos mantêm sua direção individual. A direção também estiliza fundo, títulos gerados e legendas. Arquivos de fonte e estilos por cena ainda não são carregados.

Na timeline do projeto gerado, a faixa superior mostra os shots e os marcadores dos beats. Clique para ir ao início do plano/evento. Arraste a alça verde à esquerda de um shot para mover o corte. Os elementos, beats e keyframes de câmera do plano seguinte acompanham o corte; uma operação de arraste é desfeita em um único passo. O inspetor grava keyframes de câmera e edita transição/duração no shot ativo.

Um `beat` exige `id`, `at` e `action`. `at` usa a mesma referência de fala do início do shot e também aceita `wordIndex` para sincronizar com o início de uma palavra reconhecida pelo sintetizador local. O índice é zero-based dentro do trecho `cue`; índices fora do alinhamento real são rejeitados. `show` e `hide` exigem `target` e mudam a opacidade do elemento no instante indicado. `transform` exige `target` e ao menos um de `x`, `y`, `rotation`, `scale` ou `opacity`; `durationFrames` opcional (0–300) controla a duração da mudança. `camera` altera `x`, `y`, `rotation` e/ou `zoom`, pode animar por `durationFrames` e não usa `target`. Todos os alvos devem ser elementos daquele shot; beats fora do intervalo são rejeitados. Beats de câmera e transformações são compilados como keyframes para manter o projeto editável.

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

A receita a seguir é o Director's Cut de 90 segundos: seis cenas, dezoito shots, dezenove Visual Beats e timestamps resolvidos pela voz pt-BR. O smoke de produção sintetiza a narração, renderiza 2700 quadros e compara os quadros de shots/beats da exportação com a prévia. O atualizador do guia insere aqui o arquivo de exemplo real, sem reconstruir ou inventar seus campos.

<!-- STUDIO-DARK-VALIDATED-EXAMPLE-BEGIN -->
<!-- STUDIO-DARK-VALIDATED-EXAMPLE-END -->

## Composição, grupos e câmera

Não há cabeçalho, título de cena ou cenário forçado em v2. `title`, `kicker` e `background` são opcionais por cena. Sem fundo nativo, o compositor mantém seu gradiente escuro base. Posicione texto e objetos onde fizer sentido; legendas automáticas ocupam a faixa inferior.

`group` exige `children` e pode aninhar até 8 níveis. Coordenadas dos filhos são locais ao canto superior esquerdo do grupo pai. A ordem da lista determina a ordem de desenho. Um grupo pode ter `x`, `y`, `width`, `height`, `rotation`, `scale`, `opacity`, `cue`, `offsetSeconds` e `span` como outro elemento.

`keyframes` em elementos são estados de `x`, `y`, `rotation`, `scale` e `opacity`. Cada chave usa `cue` e `offsetSeconds`; propriedades omitidas mantêm o valor inicial daquele elemento. Opcionalmente, `easing` na **chave de destino** escolhe a interpolação até ela: `linear` (padrão), `ease-in`, `ease-out` ou `ease-in-out`. A primeira chave não tem trecho anterior. As chaves devem cair dentro do trecho de vida do elemento. Evite combinar `keyframes` com `motion`, `travelX`, `travelY` ou `spinDegrees` no mesmo item.

`camera` é uma lista de keyframes na cena com `cue`, `x`, `y`, `zoom` e `rotation`; `offsetSeconds` e `easing` são opcionais. A curva tem a mesma semântica da chave de destino usada nos elementos. O centro inicial é 640×360, zoom 1 e rotação 0. A câmera atravessa os limites entre cenas e mantém o último estado até a próxima chave. Zoom válido: 0.1 a 10. `cue` começa em zero em cada cena.

Para sincronizar, informe `cue` e opcionalmente `offsetSeconds` de 0 a 10. O compilador mede o áudio real. `span` aceita `scene` (padrão) ou `utterance`. Não use timestamps inventados.

## Elementos

`kind`: `group`, `library`, `native`, `text`, `shape`, `image`, `svg` ou `video` (vídeo requer receita v3). `native` usa o campo `asset`; texto usa `text`; mídia direta usa `source`, caminho local absoluto ou relativo ao JSON. Arquivos remotos e URLs não são baixados. PNG com transparência e SVG compatível com Qt são aceitos. Vídeos locais MP4, MOV, MKV, WebM, AVI e M4V são decodificados em quadros pela mesma composição usada na prévia e exportação. Eles mantêm proporção e preenchem a caixa com corte central; os quadros são processados sob demanda com FFmpeg.

Propriedades: `x`, `y`, `width`, `height`, `cueProgress` (alternativo a `offsetSeconds`), `color` (#RRGGBB), `secondaryColor`, `fontSize` (8–300), `opacity` (0–1), `rotation`, `scale` (0.01–10), `travelX`, `travelY`, `spinDegrees`, `motion`, `motionFrames` (1–300), `exitFrames` (0–300), `pose`, `cue`, `offsetSeconds`, `span`, `keyframes`, `children`, `assetId`.

Movimentos: `none`, `fade`, `rise`, `slide-left`, `slide-right`, `pop`. Poses: `neutral`, `point`, `wave`, `worried`. Roupas/personagem usam `color`; `secondaryColor` controla a pele. Partes de personagem ainda não são objetos articulados.

Biblioteca nativa disponível: car, person, house, building, document, computer, phone, coins, price-tag, bars, street, office, showroom e studio. Barras são ilustrativas e não representam números de pesquisa.

## Biblioteca pessoal desta cópia do guia

)GUIDE");
    QMap<QString, QString> categories;
    for (const auto &value : library["categories"].toArray()) {
        const auto category = value.toObject();
        categories.insert(category["id"].toString(), category["name"].toString());
    }
    QStringList assets;
    for (const auto &value : library["assets"].toArray()) {
        const auto asset = value.toObject();
        if (asset["deleted"].toBool() || asset["builtin"].toBool())
            continue;
        QString dimensions;
        if (asset["width"].isDouble() && asset["height"].isDouble())
            dimensions = ", " + QString::number(asset["width"].toInt()) + "×" +
                         QString::number(asset["height"].toInt());
        else if (asset["kind"] == "image") {
            QImageReader reader(asset["source"].toString());
            const auto size = reader.size();
            if (!size.isEmpty())
                dimensions = ", " + QString::number(size.width()) + "×" + QString::number(size.height());
        } else if (asset["kind"] == "svg") {
            QSvgRenderer svg(asset["source"].toString());
            if (svg.isValid() && !svg.defaultSize().isEmpty())
                dimensions = ", " + QString::number(svg.defaultSize().width()) + "×" +
                             QString::number(svg.defaultSize().height());
        } else if (asset["kind"] == "preset") {
            const auto nodes = asset["preset"].toObject()["nodes"].toArray();
            if (!nodes.isEmpty()) {
                const auto node = nodes.at(0).toObject();
                dimensions = ", " + QString::number(node["width"].toInt()) + "×" +
                             QString::number(node["height"].toInt());
            }
        }
        QString detail;
        if (asset["kind"] == "preset") {
            const auto nodes = asset["preset"].toObject()["nodes"].toArray();
            if (!nodes.isEmpty()) {
                const auto node = nodes.at(0).toObject();
                if (nodes.size() > 1)
                    detail = ", composição com " + QString::number(nodes.size()) + " partes";
                else if (node["kind"] == "native")
                    detail = ", objeto " + node["nativeId"].toString();
                else if (node["kind"] == "text" && node["text"].isString())
                    detail = ", texto: " + node["text"].toString().left(100);
                if (!node["keyframes"].toArray().isEmpty())
                    detail += ", raiz animada";
            }
        }
        assets.append("- `" + asset["id"].toString() + "` — " + asset["name"].toString() + " (" +
                      categories.value(asset["categoryId"].toString(), "Sem categoria") + ", " +
                      asset["kind"].toString() + dimensions + detail + ").");
    }
    if (!library["assets"].isArray())
        guide += "Esta cópia não recebeu o catálogo local. Salve o guia pelo botão no app para incluir sua "
                 "biblioteca pessoal.\n";
    else if (assets.isEmpty())
        guide += "Nenhum item pessoal ativo no momento em que este guia foi salvo.\n";
    else {
        constexpr int manifestLimit = 500;
        for (int i = 0; i < std::min(int(assets.size()), manifestLimit); ++i)
            guide += assets[i] + "\n";
        if (assets.size() > manifestLimit)
            guide += "\nManifesto limitado aos primeiros 500 itens ativos.\n";
    }
    guide += R"GUIDE(
Use um item deste manifesto com `{"kind":"library","assetId":"ID-DO-ITEM","cue":0}`. Itens apagados não aparecem. O aplicativo resolve IDs na biblioteca local e informa se o item foi removido ou não existe. Não invente IDs nem suponha que outra máquina tenha os mesmos arquivos. O manifesto descreve nomes, categorias e dimensões; ele não inspeciona visualmente a imagem. Se a descrição não bastar para decidir, pergunte ao usuário. Itens image/SVG também podem ser referenciados por `source` se o usuário fornecer um caminho.

## Presets compostos na biblioteca pessoal

O app agora permite salvar e reutilizar um **grupo raiz** com filhos, subgrupos, animações e mídias copiadas para a biblioteca. O guia exportado pelo aplicativo identifica esses presets; o guia do repositório não contém IDs pessoais.

Em receitas v2, `{"id":"painel1","kind":"library","assetId":"ID-DO-PRESET","cue":0}` também instancia grupos inteiros. Os filhos mantêm posições locais, duração e keyframes em frames relativos ao cue escolhido (30 FPS); o importador cria novos IDs e preserva a hierarquia. A duração efetiva é limitada pela cena/trecho medido: uma chave que cair depois do fim provoca erro, não um corte silencioso. Se a animação precisa ser ajustada à narração, descreva o grupo diretamente em `children` com keyframes por cue.

Propriedades visuais passadas à referência alteram a raiz; keyframes salvos continuam controlando a animação dessa raiz. Não combine keyframes explícitos com o mesmo preset animado. Se o item do manifesto for uma imagem, seu nome não confirma o conteúdo visual: solicite descrição quando necessário. IDs dependem da instalação e devem ser obtidos exportando o guia atualizado no próprio app.

## Catálogo nativo do aplicativo

)GUIDE";
    for (const auto &asset : nativeLibrary())
        guide += "- `" + QString::fromStdString(asset.id) + "` — " + QString::fromStdString(asset.name) +
                 " (" + QString::fromStdString(asset.category) + "), " + QString::number(asset.width) + "×" +
                 QString::number(asset.height) + ".\n";
    guide += R"GUIDE(

`musicGain`: 0 a 0.08; zero desliga a trilha procedural. `effectsGain`: 0 a 0.15; zero desliga sinais de troca de cena. `sources` pode registrar título e URL HTTPS; é referência editorial, não verificação factual. Revise fatos, fontes e direitos de mídia antes de publicar.

A receita v1 antiga continua sendo lida, com cabeçalho/cenário/layout legado. Receitas v2 mantêm grupos, presets, câmera e keyframes; v3 acrescenta shots, beats, camadas de vídeo, fades e automação de áudio. Campos desconhecidos, IDs duplicados, itens pessoais inativos, arquivos ausentes e limites fora da faixa são rejeitados com mensagem. Projetos v1–v8 continuam abrindo; camadas de vídeo usam schema 5, fades de pista não nulos usam schema 6, legendas com destaque sincronizado por palavra usam schema 7 e automação de ganho usa schema 8.

## Checklist de limites e publicação

O app não baixa mídias da Internet nem gera automaticamente imagens ou vídeos a partir de um prompt. Caminhos relativos de `source` são relativos ao arquivo `.production.json`, em UTF-8. `sources` com URLs HTTPS guarda **referências editoriais**, não verifica fatos nem cria citações visuais. Confira direitos de uso e dados antes de publicar.

Vídeo local como camada, sincronização por palavra em shots/beats/elementos/keyframes e destaque palavra a palavra nas legendas, biblioteca vetorial, áudio em várias pistas e automação de ganho por pontos estão implementados. Continuam pendentes extração automática do áudio embutido no vídeo, trim/velocidade/recorte manual de clipes, documentos/PDF com regiões, conectores ancorados, trajetórias Bézier personalizadas, desenho livre de envelopes, efeitos sonoros disparados por beat e ferramentas avançadas de timeline. Não descreva essas funções pendentes como disponíveis no JSON.

## Capacidades em JSON

```json
)GUIDE";
    guide += QString::fromUtf8(QJsonDocument(capabilities()).toJson());
    guide += "```\n";
    return guide;
}
} // namespace studio
