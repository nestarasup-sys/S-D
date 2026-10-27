#include <QImageReader>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QStringList>
#include <QSvgRenderer>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>
#include <studio/storage/workspace_store.hpp>

namespace studio {
QJsonObject capabilities() {
    initializePremiumCatalog();
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
    QJsonArray premiumItems;
    for (const auto &asset : premiumLibrary()) {
        QJsonArray slotRoles, slotDefinitions, paletteTokens;
        for (const auto &role : asset.slotRoles)
            slotRoles.append(QString::fromUtf8(role.data(), int(role.size())));
        for (const auto &token : asset.paletteTokens)
            paletteTokens.append(QString::fromUtf8(token.data(), int(token.size())));
        QFile manifest(QStringLiteral(":/studio/premium/") +
                       QString::fromStdString(asset.manifestResource));
        if (manifest.open(QIODevice::ReadOnly)) {
            const auto document = QJsonDocument::fromJson(manifest.readAll());
            for (const auto &slotValue : document.object().value("slots").toArray()) {
                const auto slot = slotValue.toObject();
                QJsonObject definition{{"id", slot.value("id")}, {"kind", slot.value("kind")},
                                       {"nodeId", slot.value("nodeId")}, {"role", slot.value("role")},
                                       {"optional", slot.value("optional").toBool(false)}};
                if (slot.value("maxLength").isDouble())
                    definition.insert("maxLength", slot.value("maxLength"));
                if (slot.value("textLayout").isObject())
                    definition.insert("textLayout", slot.value("textLayout"));
                slotDefinitions.append(definition);
            }
        }
        premiumItems.append(QJsonObject{{"id", QString::fromUtf8(asset.id.data(), int(asset.id.size()))},
                                        {"name", QString::fromUtf8(asset.name.data(), int(asset.name.size()))},
                                        {"kind", QString::fromUtf8(asset.type.data(), int(asset.type.size()))},
                                        {"manifestVersion", asset.manifestVersion}, {"slots", slotRoles},
                                        {"slotDefinitions", slotDefinitions},
                                        {"paletteTokens", paletteTokens},
                                        {"responsiveCanvases", QJsonArray{"1280x720", "1920x1080", "1080x1920"}},
                                        {"status", "implemented"}});
    }
    return {{"format", "studio-dark-capabilities"},
            {"catalogVersion", 1},
            {"productionVersion", 3},
            {"projectVersion", 15},
            {"productionCapabilities",
             QJsonArray{"free-scene-layout", "nested-groups", "v3-scene-title-metadata", "transform-keyframes-linear",
                        "camera-2d-keyframes", "keyframe-easing-curves", "personal-library-assets",
                        "reusable-group-presets", "optional-visual-captions", "fixed-duration-timing",
                        "automatic-duration-extension", "cue-progress-sync", "pitch-preserving-atempo", "independent-shots",
                        "visual-beats-show-hide-transform-camera-sound", "instant-visual-beat-show-hide",
                        "shot-cut-camera",
                        "narration-content-shot-rhythm",
                        "shot-transitions-fade-dissolve-slide-zoom", "auto-reframe-by-shot-target",
                        "continuous-shot-focus-follow", "intensity-driven-default-motion",
                        "reusable-camera-keyframe-presets",
                        "global-art-direction", "scene-shot-art-direction", "word-aligned-visual-beats",
                        "external-library-fonts", "named-visual-style-presets",
                        "declarative-premium-templates", "premium-content-slot-overrides",
                        "premium-image-slot-overrides", "camera-path-assisted-framing",
                        "camera-spatial-reveal", "camera-spatial-and-temporal-reveal",
                        "premium-slot-adaptive-text-layout", "visual-quality-analysis-before-export",
                        "camera-path-target-framing-diagnostics", "premium-slot-image-relative-paths",
                        "embedded-video-audio-with-source-trim-and-speed",
                        "word-aligned-elements-keyframes",
                        "multi-track-audio", "visual-beat-sound-effects",
                        "voice-sidechain-ducking", "audio-track-fades", "audio-gain-automation-easing",
                        "premium-palette-overrides", "premium-factory-editorial-templates",
                        "video-layer-broll", "video-source-trim", "video-speed-control", "video-normalized-crop",
                        "personal-library-video-assets",
                        "landscape-and-portrait-canvas-presets",
                        "word-timed-karaoke-captions", "word-animated-karaoke-captions",
                        "independent-caption-highlight-color"}},
            {"projectCapabilities",
             QJsonArray{"group-hierarchy", "transform-keyframes-linear", "camera-2d-keyframes",
                        "keyframe-easing-curves", "shot-ownership", "visual-beat-markers", "audio-tracks",
                        "visual-beat-linked-audio-tracks",
                        "video-layers", "video-trim-speed-crop", "scene-art-direction", "shot-art-direction",
                        "audio-track-fades", "audio-gain-automation",
                        "word-timed-caption-highlights", "word-animated-caption-presets",
                        "independent-caption-highlight-color"}},
            {"width", 1280},
            {"height", 720},
            {"supportedCanvases", QJsonArray{
                 QJsonObject{{"name", "Paisagem HD"}, {"width", 1280}, {"height", 720}},
                 QJsonObject{{"name", "Paisagem Full HD"}, {"width", 1920}, {"height", 1080}},
                 QJsonObject{{"name", "Retrato"}, {"width", 1080}, {"height", 1920}}}},
            {"fps", 30},
            {"nativeAssets", items},
            {"premiumAssets", premiumItems},
            {"premiumAssetCategories", QJsonArray{"basic-native", "premium", "real-media"}},
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
            {"maxNodes", QJsonValue::Null},
            {"nodeLimitPolicy", "resource-bound"},
            {"maxNarrationCharactersPerCue", 150},
            {"audio", QJsonArray{"windows-pt-BR-tts", "windows-sapi-word-boundaries",
                                 "procedural-bed", "scene-chime", "multi-track-wav-mp3-aac-ogg-flac-wma",
                                 "voice-sidechain-ducking"}}};
}
QString aiGuide(const QJsonObject &library) {
    initializePremiumCatalog();
    QString guide = QString::fromUtf8(R"GUIDE(# Studio Dark — guia para dirigir um vídeo com IA

Catálogo 1 · Receita 3 · Projeto 14 · Native Lab 0.5

Você é o diretor audiovisual. Transforme o roteiro em uma produção editável. Preserve o texto do usuário, salvo pedido explícito de reescrita. Escolha enquadramentos e composições livres usando somente os recursos abaixo. Entregue JSON puro, sem cercas Markdown.

## Fluxo

O usuário escreve o roteiro fora do aplicativo, fornece este guia à IA e salva a resposta em `.production.json`. **Importar produção IA** gera voz local, mede cada fala, resolve os cues, monta o projeto e sincroniza o áudio. A pessoa pode editar cada grupo e elemento no editor e exportar o vídeo. Não invente duração nem timestamps da fala.

Receitas v1 e v2 continuam aceitas por compatibilidade. Para novas produções com cortes dentro da mesma fala, use v3.

## Como planejar a receita

- Responda **somente com JSON válido**, sem comentários, cercas Markdown nem texto de apresentação. Use `format: "studio-dark-production"` e `catalogVersion: 1`; escolha `version: 3` para shots/Visual Beats ou `version: 2` para a composição de cena tradicional.
- Preserve o roteiro recebido, salvo instrução de reescrever. Divida cada cena em trechos faláveis, de no máximo 150 caracteres, e use `cue` como índice (a partir de zero) na narração da **mesma cena**.
- `offsetSeconds` varia de 0 a 10; `cueProgress` usa de 0 a 1 para acompanhar a fala final. Sem duração fixa, a duração natural depende da voz pt-BR e só é conhecida depois de sintetizar o WAV.
- Monte a narrativa com composição livre, `group`, `shape`, `text`, `native`, `image`, `svg`, `video`, biblioteca pessoal, câmera e keyframes documentados. Reserve espaço para as legendas se `captions` for `true`.
- Não há teto fixo de objetos. Grupos, presets premium e legendas geradas contam como camadas e consomem CPU, RAM e tempo de renderização conforme a complexidade; IDs devem permanecer únicos. Sem `canvas`, use 1280×720; nos outros formatos aceitos, componha elementos dentro das dimensões escolhidas e mantenha margens para legendas. Grupos de receita continuam limitados a oito níveis.
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

Use de 1 a 20 cenas. Cada cena tem de 1 a 10 trechos de narração, até 150 caracteres cada. Não existe limite fixo agregado de elementos: expansões de grupos, recursos premium e legendas podem aumentar o projeto e o custo de CPU, RAM, importação e renderização. A aplicação valida IDs, referências, profundidade e limites próprios de duração/texto. Áudio por voz usa uma voz pt-BR instalada no Windows; com `voice: "auto"`, prefere uma voz masculina e usa outra voz pt-BR se nenhuma masculina estiver instalada. Também é possível informar o nome exato da voz instalada. `speechRate` vai de -3 a 3.

`captions` controla as legendas visuais automáticas (padrão `true`). O arquivo SRT ainda é criado mesmo quando `captions` é `false`. Quando a voz Windows pt-BR emite alinhamento real, a legenda visual destaca a palavra falada com `artDirection.captionHighlightColor`; quando omitida, herda `primaryColor`. A cor pode ser configurada globalmente, por cena ou por shot. O projeto guarda os intervalos em quadros no schema 7 e a cor independente no schema 14. Depois da importação, o inspetor permite ajustar início e fim de cada palavra alinhada numericamente; na faixa da legenda da timeline, arraste um limite para aproximar ao quadro mais próximo. Limites compartilhados mantêm palavras vizinhas contíguas e um gesto inteiro pode ser desfeito. Esses ajustes ficam no projeto compilado e não são campos de entrada da receita. Sem alinhamento real, a legenda continua estática. O SRT não contém cores.

`wordAnimation` é opcional na raiz v3 e aplica movimento à palavra ativa de cada legenda alinhada. Presets: `highlight` (padrão, somente cor), `none`, `pop`, `rise`, `fade` e `pop-rise`. Exemplo: `{"wordAnimation":{"preset":"pop-rise","durationFrames":8,"startScale":0.82,"travelY":12}}`. `durationFrames` aceita 1–30 (padrão 8), `startScale` aceita 0.5–1.5 (padrão 0.82) e `travelY` aceita −80 a 80 px (padrão 12; positivo sobe). O app reconcilia os eventos da voz com as palavras escritas, inclusive quando números e percentuais são pronunciados como várias palavras; palavras que caem no mesmo quadro são agrupadas. O projeto salva os parâmetros no schema 12; inspetor, prévia e exportação usam o mesmo preset. A escala e a subida terminam no tamanho/posição normal; `fade` aumenta a opacidade durante o intervalo inicial.

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

**Campos:** `timingMode` aceita `natural` ou `fit-to-duration`; para o segundo, `targetDurationSeconds` é obrigatório (1–600 s), e `preserveVoicePitch` só aceita `true` nesta versão. Em cada cena, `targetDurationSeconds` é opcional; em cada entrada de `narration`, um objeto `{"text":"...","targetDurationSeconds":2.5}` define uma meta de intervalo **incluindo voz e pausa**. Strings antigas continuam funcionando. O alvo global, os alvos de cena e os alvos individuais de fala são metas: se a fala não couber com aceleração de até 1,15×, o app amplia os intervalos necessários e a duração final, registrando um aviso.

O Studio Dark sintetiza cada áudio primeiro, mede amostras a 24 kHz e depois planeja intervalos inteiros de quadros (30 FPS). O `atempo` do FFmpeg acelera apenas a fala que não cabe, buscando preservar seu tom, com limite rígido de **1,15×**. Se uma meta global, de cena ou de fala for curta, o app amplia os intervalos necessários e mostra um aviso com a duração final; não descarta falas nem ultrapassa o limite de velocidade. Quando sobra tempo, insere silêncio como pausa; não estica a voz para preencher. Requer FFmpeg e voz pt-BR local no Windows.

Para sincronizar com fala ajustada, use `{"cue":0,"cueProgress":0.5}` no elemento, keyframe ou câmera. `cueProgress` vai de 0 (início da fala) a 1 (último quadro audível); 0,5 significa a metade temporal da **fala ajustada**, não da cena nem de sua pausa. Não misture `cueProgress` e `offsetSeconds` no mesmo objeto. O `offsetSeconds` de 0 a 10 continua disponível para deslocamentos absolutos. A receita v3 também aceita `wordIndex` onde documentado abaixo; a resolução vem dos eventos reais da voz pt-BR.

**Planejamento:** para 90 s, 2700 quadros a 30 FPS. A IA pode planejar mas não conhece a duração de uma voz local ainda não gerada. Evite silêncios excessivos; o aplicativo mede a narração e amplia a duração final se for necessário para respeitar o limite de aceleração.

## Receita v3 — cenas, shots e Visual Beats

### Formato do quadro

Receitas v1/v2 e receitas v3 sem `canvas` continuam usando 1280×720. Uma receita v3 pode declarar `"canvas":{"width":1080,"height":1920}` para vídeo vertical ou escolher 1280×720 e 1920×1080 para paisagem. `canvas` é um objeto opcional da raiz v3; `width` e `height` são inteiros obrigatórios quando o objeto está presente e nenhum outro campo é aceito. São aceitos somente estes pares: 1280×720 (padrão), 1920×1080 e 1080×1920. Dimensões e coordenadas dos elementos são pixels do quadro escolhido. Câmeras sem centro explícito começam no centro do quadro. O projeto mantém a resolução escolhida na edição, prévia e exportação. Cenários nativos de 16:9 preservam a proporção e são cortados nas laterais para preencher o formato vertical. Exemplo completo: `examples/retrato-1080x1920-v3.production.json`.

Uma cena narrativa pode conter vários shots sem repetir nem interromper a narração. A versão 3 aceita as configurações globais da v2 e substitui `elements` e `camera` da cena por `shots`. Cada cena v3 exige de 1 a 64 shots; a produção aceita até 256 shots e 4096 Visual Beats. O primeiro shot começa junto ao início da cena. O início do shot seguinte encerra o anterior; o último termina com a cena. Assim, os planos cobrem a cena sem lacunas ou sobreposições.

Cada cena v3 também pode declarar `audioTracks` (até 32 no projeto). Uma pista usa `id`, `source` (arquivo de áudio local relativo ao JSON ou absoluto; também pode ser um vídeo local que contenha áudio), `start` (a mesma referência a cue/progresso/wordIndex), `gain` (0–4), `duckUnderVoice` e `duckAmount` (0–1). `name` é opcional. `sourceInSeconds` (0–3600, padrão 0) corta a fonte antes da reprodução; `playbackRate` (0,25–4, padrão 1) controla sua velocidade sem alterar a duração da pista na timeline. Em fontes de vídeo, a faixa de áudio 0 é usada. `fadeInFrames` e `fadeOutFrames` são opcionais (0–1800) e aplicam fades lineares ao começo/fim da pista; a soma deve caber no intervalo disponível. `gainAutomation` é opcional e aceita até 128 pontos `{"frame":0,"value":0.5,"easing":"linear"}`; o quadro é relativo ao início da pista, `value` multiplica o ganho base (0–4), e `easing` aceita `linear`, `ease-in`, `ease-out` ou `ease-in-out`. Os pontos devem estar em ordem crescente. Antes do primeiro e depois do último ponto, mantém-se o nível mais próximo. A pista começa no ponto indicado e segue até o fim da cena. `duckUnderVoice:true` usa o WAV de voz isolado como referência para reduzir a pista durante a fala; sem stem dedicado, usa `audioSource`. Quando a referência é WAV PCM mono a 24 kHz, prévia e exportação usam a mesma envoltória RMS por quadro. Outros formatos usam o sidechain FFmpeg na exportação; para sincronização idêntica à prévia, forneça um WAV compatível. Pistas ficam editáveis no projeto; na timeline é possível ajustar início/duração, ganho, trim e velocidade, adicionar/substituir pontos no playhead e desenhar o envelope: clique na faixa para criar um ponto ou arraste um ponto para ajustar posição e multiplicador (0–4). O easing escolhido no controle da pista é aplicado durante o gesto; a ação inteira pode ser desfeita de uma vez. Limpar a automação remove todos os pontos.

Cada shot exige `id`, `camera` e `elements`; `name`, `transitionIn` e `cameraPreset` são opcionais. Em ritmo manual, `start` também é obrigatório e usa `cue` e um único campo entre `cueProgress` (0–1), `progress` (alias compatível), `offsetSeconds` (0–10) ou `wordIndex` (índice da palavra, começando em zero). `wordIndex` usa os eventos de progresso emitidos pela voz Windows pt-BR e não pode ser combinado com outros deslocamentos. O tempo é resolvido sobre o áudio medido. Com `editing.rhythm:"narration"`, omita `start` de todos os shots; o compilador escolhe cortes depois de palavras alinhadas, favorecendo pontuação e pivôs de frase como “mas”, “porém” e “então”, enquanto equilibra a duração dos planos conforme `editing.intensity`. A seleção não inventa timestamps: toda fala da cena precisa ter um timestamp real por palavra e precisam existir pontos suficientes para acomodar todos os shots, a duração mínima de cada plano e suas transições. Se o alinhamento ou os limites não forem suficientes, a produção informa o motivo. Elementos do shot sem referência temporal explícita começam no corte calculado, inclusive filhos de grupos; horários explícitos de elementos, beats e keyframes continuam respeitados e precisam caber no intervalo do shot. Não misture início automático e manual na mesma receita. `transitionIn` pode ser a string legada `cut`, um objeto `{"type":"cut"}` ou um objeto de transição animada. Um `durationFrames` opcional junto a `type:"cut"` é aceito por compatibilidade e normalizado para corte imediato. O primeiro shot da cena sempre começa sem transição. Transições animadas sobrepõem visualmente o último quadro do shot anterior e o novo shot, sem alterar a duração da narração nem o tempo total da cena.

Exemplos de transição: `{"type":"fade","durationFrames":18}`, `{"type":"dissolve","durationFrames":12}`, `{"type":"slide","direction":"left","durationFrames":16}` ou `{"type":"zoom","durationFrames":12}`. `durationFrames` aceita de 2 a 300 quadros e precisa caber no shot de entrada. `slide` aceita direção `left`, `right`, `up` ou `down`. `fade` atravessa preto; `dissolve` mistura as imagens; `slide` move o novo shot; `zoom` aproxima o novo shot ao entrar. Prévia e exportação usam o mesmo compositor. A timeline destaca a faixa de transição.

`camera` aceita `x`, `y`, `zoom` e `rotation`; quando omitidos, o centro corresponde ao canvas escolhido, o zoom é 1 e a rotação é 0.

Presets reutilizáveis de câmera são definidos uma vez em `cameraPresets` na raiz v3 e referenciados com `cameraPreset` em qualquer shot. Há limite de 32 nomes e 512 keyframes nas definições. Cada nome usa letras, números, `_` ou `-` e aponta para 2–64 estados completos de câmera. `offsetSeconds` é relativo ao início do shot, começa em zero e avança pelo menos um quadro; cada estado também exige `x`, `y`, `zoom` (0.1–10), `rotation` (−360 a 360) e `easing` opcional (`linear`, `ease-in`, `ease-out` ou `ease-in-out`). `x` e `y` aceitam −10000 a 10000. Exemplo: `{"cameraPresets":{"push-in":[{"offsetSeconds":0,"x":640,"y":360,"zoom":1,"rotation":0},{"offsetSeconds":1.2,"x":640,"y":360,"zoom":1.25,"rotation":0,"easing":"ease-in-out"}]}}`, com `"cameraPreset":"push-in"` no shot. O estado de offset zero substitui a câmera inicial daquele shot; os demais valores viram keyframes editáveis no projeto. O último estado precisa caber no intervalo do shot. O mesmo preset pode ser usado em vários shots. Não combine com `camera.focus.follow:true` nem com Visual Beats de ação `camera` no mesmo shot. O compilador limita a 4096 keyframes expandidos na produção.

Para enquadrar automaticamente um elemento ou grupo, adicione `"focus":{"target":"ID-DO-ELEMENTO","padding":1.25}` dentro de `camera`. O alvo deve pertencer ao mesmo shot. O aplicativo transforma os limites locais através de todos os grupos ancestrais, centraliza a câmera nesse retângulo e calcula o zoom para mantê-lo dentro do quadro com a margem solicitada. `padding` aceita 1–4 e assume 1.25. Quando `focus` está presente, ele substitui o centro e o zoom explícitos; a rotação continua sendo usada. Com `"follow":true`, dentro de `focus`, o aplicativo recalcula o enquadramento para cada quadro do shot e grava a trajetória como keyframes editáveis. Para compatibilidade, `camera.follow` no nível da câmera também é aceito. `follow` assume false. Não combine esse modo com Visual Beats de ação `camera` no mesmo shot.

Elementos declarados dentro do shot pertencem apenas a ele e viram faixas visuais editáveis no projeto. Sem referência temporal explícita, começam no início do shot. Os elementos e grupos continuam usando os tipos e propriedades documentados abaixo.

`editing` é opcional na receita v3. `intensity` aceita inteiro de 0 a 100 e só preenche `motion`, `motionFrames` e `exitFrames` omitidos nos elementos: 0 desliga movimentos automáticos, 1–30 usa entradas discretas com fade, 31–70 usa entradas rise e 71–100 usa pop mais rápido. Campos explicitamente declarados pela IA não são substituídos. `rhythm` aceita `manual` (padrão) ou `narration`; o modo narration requer inícios de shot omitidos em toda a receita e alinhamento real de cada palavra. Ele seleciona cortes em limites de frases e pivôs de fala disponíveis, buscando equilíbrio entre ritmo e pontuação; a intensidade ajusta esse equilíbrio. Sem timestamps suficientes ou duração para distribuir os shots, a produção falha com uma mensagem específica. Sem o objeto `editing`, receitas v3 mantêm os defaults anteriores; receitas v1/v2 não aceitam essa opção.

`artDirection` é opcional na raiz da receita v3 e define o estilo global. Os mesmos campos podem ser informados parcialmente em cada cena (`scenes[].artDirection`) e shot (`scenes[].shots[].artDirection`). A herança é global → cena → shot; cada objeto local substitui apenas os campos presentes. Valores `color`, `secondaryColor` e `fontSize` explícitos nos elementos mantêm prioridade sobre os defaults de direção de arte. O estilo efetivo controla fundo, tipografia, cartões padrão, cabeçalhos, legendas e destaque da palavra. O inspetor permite escolher escopo global, cena atual ou shot atual. `backgroundColor`, `glowColor`, `primaryColor`, `secondaryColor`, `textColor`, `cardColor`, `captionBackgroundColor` e `captionHighlightColor` usam `#RRGGBB`; `fontFamily` aceita nome de fonte instalado (até 120 caracteres); `titleSize` e `bodySize` aceitam 8–160 px; `captionSize` aceita 8–120 px; `captionPosition` aceita `top` ou `bottom`; `safeMargin` aceita 0–300 px. O campo `artDirectionPreset` aceita o ID de um item `visual-style` da biblioteca pessoal na raiz, cena ou shot; campos inline sobrescrevem o preset. Fontes importadas ficam disponíveis localmente e também podem ser usadas em `fontFamily`.

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

Um `beat` exige `id`, `at` e `action`. `at` usa a mesma referência de fala do início do shot e também aceita `wordIndex` para sincronizar com o início de uma palavra reconhecida pelo sintetizador local. O índice é zero-based dentro do trecho `cue`; índices fora do alinhamento real são rejeitados. `show` e `hide` exigem `target` e atuam instantaneamente no quadro resolvido: `show` torna visível um elemento mesmo que seu `opacity` inicial seja 0; `hide` o oculta. O `show` restaura a última opacidade positiva conhecida do elemento; se nenhuma existir, usa 1. Uma opacidade inicial semitransparente é preservada. `hide` seguido de `show` restaura esse mesmo valor. Eventos instantâneos mantêm o estado até o próximo beat ou o fim do shot e não interpolam antes do quadro marcado. Beats no mesmo quadro seguem a ordem declarada na receita. `transform` exige `target` e ao menos um de `x`, `y`, `rotation`, `scale` ou `opacity`; `durationFrames` opcional (0–300) controla a duração da mudança: zero é instantâneo, valor positivo interpola do estado atual ao destino. `camera` altera `x`, `y`, `rotation` e/ou `zoom`, pode animar por `durationFrames` e não usa `target`. Todos os alvos devem ser elementos daquele shot; beats fora do intervalo são rejeitados. Beats de câmera e transformações são compilados como keyframes para manter o projeto editável.

Exemplos executáveis no parser e no compilador: [revelação sequencial de três objetos](../examples/visual-beats-sequential-reveal-v3.production.json) (`opacity: 0`, `show` em 20%, 46% e 73% da mesma fala) e [transformação, câmera, som e hide/show](../examples/visual-beats-transform-camera-v3.production.json). No primeiro, cada objeto permanece visível depois do seu instante de revelação. O alinhamento `cueProgress` usa a duração real da fala sintetizada; `wordIndex` só funciona quando o sintetizador forneceu timestamps reais.

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
<!-- STUDIO-DARK-VALIDATED-EXAMPLE-END -->

## Composição, grupos e câmera

Não há cabeçalho ou cenário forçado em v2. `title`, `kicker` e `background` são opcionais por cena. Em v3, `title` opcional nomeia a cena na timeline/projeto e não é desenhado por cima dos shots; crie cabeçalhos visuais como elementos nos próprios shots. `kicker` e `background` não são campos de cena v3. Sem fundo nativo, o compositor mantém seu gradiente escuro base. Posicione texto e objetos onde fizer sentido; legendas automáticas ocupam a faixa inferior. Texto comum, títulos e legendas são desenhados no espaço da composição e acompanham pan, zoom e rotação da câmera; esta versão não possui uma camada de texto presa ao viewport. Ao anunciar uma técnica enquanto a câmera percorre conteúdo abaixo, mantenha ambos no enquadramento e reserve margem segura para a legenda.

`group` exige `children` e pode aninhar até 8 níveis. Coordenadas dos filhos são locais ao canto superior esquerdo do grupo pai. A ordem da lista determina a ordem de desenho. Um grupo pode ter `x`, `y`, `width`, `height`, `rotation`, `scale`, `opacity`, `cue`, `offsetSeconds` e `span` como outro elemento.

`keyframes` em elementos são estados de `x`, `y`, `rotation`, `scale` e `opacity`. Cada chave usa `cue` e `offsetSeconds`; propriedades omitidas mantêm o valor inicial daquele elemento. Opcionalmente, `easing` na **chave de destino** escolhe a interpolação até ela: `linear` (padrão), `ease-in`, `ease-out` ou `ease-in-out`. A primeira chave não tem trecho anterior. As chaves devem cair dentro do trecho de vida do elemento. Evite combinar `keyframes` com `motion`, `travelX`, `travelY` ou `spinDegrees` no mesmo item.

`camera` é uma lista de keyframes na cena com `cue`, `x`, `y`, `zoom` e `rotation`; `offsetSeconds` e `easing` são opcionais. A curva tem a mesma semântica da chave de destino usada nos elementos. O centro inicial é o centro do canvas, zoom 1 e rotação 0. A câmera atravessa os limites entre cenas e mantém o último estado até a próxima chave. Zoom válido: 0.1 a 10. `cue` começa em zero em cada cena. Não confunda as cinco operações: `transitionIn` muda entre shots; `motion` anima a entrada/saída padrão de um elemento; `keyframes` definem estados temporais editáveis; Visual Beats disparam ações pontuais sincronizadas à fala; câmera enquadra todo o conteúdo no espaço da composição.

Para sincronizar, informe `cue` e opcionalmente `offsetSeconds` de 0 a 10. O compilador mede o áudio real. `span` aceita `scene` (padrão) ou `utterance`. Não use timestamps inventados.

## Elementos

`kind`: `group`, `library`, `native`, `text`, `shape`, `image`, `svg` ou `video` (vídeo requer receita v3). `native` usa o campo `asset`; texto usa `text`; mídia direta usa `source`, caminho local absoluto ou relativo ao JSON. Arquivos remotos e URLs não são baixados. PNG com transparência, JPEG, WebP e SVG compatível com Qt são aceitos; a instalação oficial do projeto inclui o plugin Qt Image Formats para decodificar WebP. Vídeos locais MP4, MOV, MKV, WebM, AVI e M4V são decodificados em quadros pela mesma composição usada na prévia e exportação. Eles mantêm proporção e preenchem a caixa com corte central; o decodificador reutiliza um cache limitado a 64 MiB e faz busca direta em saltos longos. O ajuste do seek respeita o primeiro quadro real depois do trim, mantendo a mesma cadência da decodificação sequencial. Nenhum campo extra de receita é necessário.

Um vídeo pode incluir `video: {"sourceInSeconds":1.25,"durationFrames":120,"speed":1.5,"crop":{"x":0.1,"y":0.1,"width":0.8,"height":0.8}}`. `sourceInSeconds` começa a leitura nessa posição da fonte (0–3600 s); `durationFrames` define a duração de saída em quadros da timeline (1–18000), incluindo o efeito da velocidade; `speed` controla a velocidade (0.25–4, padrão 1); o recorte normalizado usa origem `x`,`y` e tamanho `width`,`height` entre 0 e 1, dentro da imagem. Se `durationFrames` for omitido, o vídeo ocupa o restante do intervalo do shot/cena. O inspector também permite editar a duração do clipe diretamente na timeline. Sem `video`, o comportamento antigo de início zero, velocidade 1 e enquadramento central permanece. Projetos com trim/fonte, velocidade ou crop personalizado usam schema 10; versões anteriores continuam abrindo.

Propriedades: `x`, `y`, `width`, `height`, `cueProgress` (alternativo a `offsetSeconds`), `color` (#RRGGBB), `secondaryColor`, `fontSize` (8–300), `opacity` (0–1), `rotation`, `scale` (0.01–10), `travelX`, `travelY`, `spinDegrees`, `motion`, `motionFrames` (1–300), `exitFrames` (0–300), `pose`, `cue`, `offsetSeconds`, `span`, `keyframes`, `children`, `assetId`.

Movimentos: `none`, `fade`, `rise`, `slide-left`, `slide-right`, `pop`. Poses: `neutral`, `point`, `wave`, `worried`. Roupas/personagem usam `color`; `secondaryColor` controla a pele. Partes de personagem ainda não são objetos articulados.

Biblioteca nativa vetorial: 72 recursos, incluindo objetos, tecnologia, comunicação, finanças, ciência, gráficos e cenários. Os símbolos e fundos são ilustrações procedurais editáveis, não fotografias. Consulte os IDs e as dimensões em **Catálogo nativo do aplicativo**. Gráficos, percentuais, mapas e painéis são apenas ilustrativos; não implicam dados externos verificados.

## Biblioteca pessoal desta cópia do guia

)GUIDE");
    QMap<QString, QString> categories;
    for (const auto &value : library["categories"].toArray()) {
        const auto category = value.toObject();
        categories.insert(category["id"].toString(), category["name"].toString());
    }
    QStringList assets, fonts, styles;
    for (const auto &value : library["assets"].toArray()) {
        const auto asset = value.toObject();
        if (asset["deleted"].toBool() || asset["builtin"].toBool())
            continue;
        if (asset["kind"] == "font") {
            fonts.append("- `" + asset["family"].toString() + "` — `" + asset["id"].toString() + "` (" +
                         asset["name"].toString() + ").");
            continue;
        }
        if (asset["kind"] == "visual-style") {
            styles.append("- `" + asset["id"].toString() + "` — " + asset["name"].toString() + ".");
            continue;
        }
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
        } else if (asset["kind"] == "preset" || asset["kind"] == "premium") {
            const auto nodes = asset["preset"].toObject()["nodes"].toArray();
            if (!nodes.isEmpty()) {
                const auto node = nodes.at(0).toObject();
                dimensions = ", " + QString::number(node["width"].toInt()) + "×" +
                             QString::number(node["height"].toInt());
            }
        }
        QString detail;
        if (asset["kind"] == "preset" || asset["kind"] == "premium") {
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
        guide += "Nenhum objeto, mídia ou composição pessoal ativo no momento em que este guia foi salvo.\n";
    else {
        constexpr int manifestLimit = 500;
        for (int i = 0; i < std::min(int(assets.size()), manifestLimit); ++i)
            guide += assets[i] + "\n";
        if (assets.size() > manifestLimit)
            guide += "\nManifesto limitado aos primeiros 500 itens ativos.\n";
    }
    guide += "\n## Fontes externas na biblioteca pessoal\n\n";
    if (fonts.isEmpty())
        guide += "Nenhuma fonte externa ativa. A direção de arte pode usar fontes disponíveis no sistema.\n";
    else
        for (const auto &font : fonts)
            guide += font + "\n";
    guide += "\n## Presets de direção de arte na biblioteca pessoal\n\n";
    if (styles.isEmpty())
        guide += "Nenhum preset visual ativo.\n";
    else
        for (const auto &style : styles)
            guide += style + "\n";
    guide += R"GUIDE(
Use um item deste manifesto com `{"kind":"library","assetId":"ID-DO-ITEM","cue":0}`. Itens apagados não aparecem. O aplicativo resolve IDs na biblioteca local e informa se o item foi removido ou não existe. Não invente IDs nem suponha que outra máquina tenha os mesmos arquivos. O manifesto descreve nomes, categorias e dimensões; ele não inspeciona visualmente a imagem. Se a descrição não bastar para decidir, pergunte ao usuário. Itens image/SVG também podem ser referenciados por `source` se o usuário fornecer um caminho.

## Categorias visuais e recursos premium

Escolha entre **Nativos básicos**, **Recursos premium** e **Imagens e vídeos reais**. As 11 categorias temáticas continuam como filtros da biblioteca. Os nove IDs premium reais são: `premium:smartphone-premium-01`, `premium:title-reveal-01`, `premium:comparison-pack-01`, `premium:dramatic-quote-01`, `premium:lower-third-editorial-01`, `premium:source-label-01`, `premium:number-emphasis-01`, `premium:question-hook-01` e `premium:statistic-callout-pack-01`. Consulte `premiumAssets` neste catálogo para nomes, tokens e `slotDefinitions` exatos.

Instancie com `{"kind":"library","assetId":"ID-REAL","cue":0}`. Todos os nove aparecem na biblioteca, têm preview renderizado, camadas editáveis, layouts para 1280×720/1920×1080/1080×1920 e usam o compositor de produção QPainter. Sete mantêm construtores C++ registrados; Question Hook e Statistic Callout são montados diretamente de manifestos declarativos v2, sem um construtor por ID.

Slots podem ser preenchidos durante a importação. Use `slots` no mesmo elemento `kind: "library"`; as chaves são os IDs curtos definidos no manifesto, e os valores são texto ou caminho local para um slot `image`. Campos desconhecidos, tipos errados, textos acima do limite ou mídia inválida causam erro com o ID do recurso/slot. Exemplo válido: `{"id":"citacao","kind":"library","assetId":"premium:dramatic-quote-01","cue":0,"slots":{"quote":"A mudança começa com uma pergunta.","attribution":"Contexto editorial","source":"Referência demonstrativa"},"palette":{"accent":"#5CD6C2","text":"#F4E1A1","muted":"#AEC3D4"}}`. `palette` e `slots` são aplicados juntos; os valores viram conteúdo dos nós daquela instância e mantêm a hierarquia e animação do template.

O Smartphone aceita slots textuais `screen-title` e `notification`, e imagem `screen`. O caminho de imagem de slot premium precisa ser relativo ao arquivo `.production.json`, sem `..`, URL, caminho absoluto, UNC ou symlink que escape da pasta da receita; usa PNG/JPEG/WebP/BMP, precisa existir e decodificar. Ao importar, o worker copia a imagem para mídia gerenciada do workspace por conteúdo. Isso mantém o projeto ligado à cópia daquele workspace, mas não cria um pacote portátil entre computadores. Os demais slots devem ser consultados em `premiumAssets[].slotDefinitions`; atualmente são textuais. Premiums não têm slot de vídeo ou ícone.

Slots de texto premium anunciam `textLayout` quando o manifesto permite ajuste. A política atual é `{"mode":"shrink","minimumFontSize":N,"padding":N}`: o compositor usa métricas Qt e quebra de linha da fonte efetiva, depois reduz a fonte apenas até o mínimo declarado. Nenhuma palavra é removida. Se a fonte diminuir, o relatório inclui uma informação `text-adapted` no primeiro quadro visível; se o texto ainda não couber, aponta o overflow. Slots sem política mantêm o comportamento anterior. `premiumAssets[].slotDefinitions[].textLayout` mostra apenas políticas reais dos manifestos.

Antes de exportar, use **Qualidade visual** ou o botão **Exportar MP4** para abrir a revisão local. Ela verifica overflow/margens de texto premium, colisões entre textos e legendas, contraste somente sobre superfícies sólidas identificáveis, beats `show` que não aparecem no momento ou surgem totalmente fora do viewport e enquadramento de alvos `camera.path`. Os apontamentos não bloqueiam a exportação e não substituem revisão artística. O botão salva `visual-quality-last.json` na pasta Logs. O worker também aceita `--quality-report <arquivo.json>` para analisar um projeto salvo sem renderizar o vídeo. `sampledFrames` lista os quadros examinados: são pontos representativos de shots, textos, câmera e beats, não uma inspeção de todos os quadros nem uma garantia de que nenhum erro visual exista.

Para testar uma receita criada por outra IA, consulte o pacote `docs/ai-evaluation/`: ele contém o desafio, a rubrica e o fluxo local de importação/análise. Nenhuma IA externa é chamada pelo aplicativo.

Personalize cores em `palette`, usando apenas `premiumAssets[].paletteTokens` e valores `#RRGGBB`. Elementos `native` aceitam `palette.primary` e `palette.secondary`. A paleta muda as camadas sem substituir o conteúdo dos slots. Veja os exemplos executáveis `examples/premium-dramatic-quote-slots-v2.production.json`, `examples/premium-comparison-slots-v2.production.json`, `examples/premium-smartphone-slots-v2.production.json`, `examples/premium-question-hook-slots-v2.production.json` e `examples/premium-statistic-callout-slots-v2.production.json`.

### Premium Factory declarativa

Definições oficiais manifest v2 em `assets/premium/*.manifest.json` podem conter `definition.definitionVersion: 1`, variantes `landscape` e `portrait`, canvas de referência e uma lista ordenada de camadas. Os componentes validados são `group`, `panel`, `rule`, `badge` e `text`; cada camada descreve layout, cor/token, papel, texto e, opcionalmente, um preset de animação suportado (`fade-rise`, `slide-left/right/up/down`, `pop-scale`, `soft-float`, `line-draw`, `stagger-reveal`). As keyframes resultantes são keyframes normais dos nós. A definição é dado validado, não código: não há scripts, caminhos de assets, plugins nem execução arbitrária. Manifesto, variantes, hierarquia, papéis, slots e tokens são comparados ao registro antes de aceitar o item.

Para novos templates compatíveis, adicione um manifesto v2 seguindo `question-hook-01.manifest.json` ou `statistic-callout-pack-01.manifest.json`; o build embute `assets/premium/*.manifest.json` automaticamente. As variantes precisam ter os mesmos IDs, pais, componentes e papéis; apenas geometria, texto, estilo e animação podem mudar entre variantes. A definição precisa passar pelo teste da biblioteca e do manifesto. Templates declarativos são instanciados por um builder genérico, sem `if assetId` ou função C++ de composição exclusiva.

Valores de Comparison Pack/Statistic Callout são demonstrativos quando assim indicados pelo template. Não apresente uma estatística demonstrativa como fato. Instalação de pacotes externos permanece fora do escopo.

### Câmera, staging e revelação

`camera.x/y` representa o centro da câmera em coordenadas da cena. Com a câmera ativa, textos, formas e nativos do shot movem-se juntos. Não existe texto fixo ao viewport. Há duas formas de revelar o próximo ponto:

- **Espacial:** o alvo já está visível no espaço da cena, mas fora do primeiro enquadramento; a câmera percorre a cena e ele entra na tela. Não use `show` para esse alvo. Se o fade padrão da propriedade `motion` o deixar transparente no instante em que a trajetória começa, use `"motion":"none"` ou posicione o evento de enquadramento depois de o alvo estar visível.
- **Temporal:** mantenha `opacity: 0` e use Visual Beat `show`; a visibilidade muda no frame do beat e persiste. A câmera pode estar parada, chegando ao alvo ou já além dele.
- **Combinada:** a câmera percorre a trajetória e um Visual Beat faz o alvo oculto aparecer durante o deslocamento. Marque o ponto com `reveal:"combined"`.

`camera.path` opcional no objeto `camera` do shot cria keyframes de câmera editáveis a partir de uma lista de alvos ordenada. Cada target exige `target` (ID de elemento ou grupo daquele shot) e `at` (referência real de `cue` com `cueProgress`, `progress`, `offsetSeconds` ou `wordIndex` alinhado); `padding` (1–4) e `easing` (`linear`, `ease-in`, `ease-out`, `ease-in-out`) são opcionais. `safeMargin` vai de 0 a 0,2 e `captionSafeBottom` de 0 a 0,35; juntos precisam deixar área visível. Use dois a 24 alvos, o primeiro deve estar no início do shot e os tempos devem crescer. O app percorre a hierarquia dos grupos, calcula bounds transformados e enquadra cada alvo respeitando margem e a faixa inferior de legendas. A ordem da IA é mantida. Cada ponto depois do primeiro vira um keyframe real; o projeto schema 15 conserva alvo, modo de revelação e margens para a análise local. Não use junto com `focus`, `focus.follow`, `cameraPreset` ou beats de câmera no mesmo shot.

O padrão é `reveal:"spatial"`: o alvo precisa estar visível no frame em que será enquadrado e não pode ter um `show` destinado a revelá-lo. Para `temporal` ou `combined`, o mesmo alvo precisa começar com opacidade efetiva zero e ter Visual Beat `show`. Um alvo temporal ainda pode ser revelado antes, durante ou depois do travelling, controlando o `cueProgress` do beat e do ponto de câmera. `camera.path` não altera o enquadramento por conta própria fora dos pontos pedidos e não faz tracking por visão computacional.

Exemplos validados pelo parser, compilador e QPainter: `examples/camera-path-spatial-v3.production.json` (entrada espacial sem beats), `examples/camera-path-combined-v3.production.json` (show durante o movimento), `examples/camera-travelling-vertical-v3.production.json`, `examples/camera-travelling-horizontal-v3.production.json`, `examples/visual-beats-sequential-reveal-v3.production.json` e `examples/premium-palette-v2.production.json`. A demonstração integrada de aproximadamente 25 segundos está em `examples/premium-factory-stage14-demo-v3.production.json`. `transitionIn` muda entre shots; `motion` anima a entrada/saída padrão; keyframes definem propriedades ao longo do tempo; Visual Beats executam ações pontuais; câmera move o enquadramento compartilhado do shot.

Para verificar o ajuste de texto e os diagnósticos de composição, `examples/visual-quality-review-demo-v3.production.json` é uma receita integrada com slots longos, paletas, comparison pack, travelling espacial e reveal combinado. `examples/visual-quality-problematic-v3.production.json` é uma fixture intencionalmente problemática: o slot da pergunta contém uma palavra sem pontos de quebra e deve gerar avisos de overflow/corte. Compare o relatório local, não use essa fixture como modelo visual.

## Presets compostos na biblioteca pessoal

O app agora permite salvar e reutilizar um **grupo raiz** com filhos, subgrupos, animações e mídias copiadas para a biblioteca. O guia exportado pelo aplicativo identifica esses presets; o guia do repositório não contém IDs pessoais.

Textos e títulos estilizados também podem ser salvos pela ação **Salvar seleção do editor**. A composição preserva texto, tamanho, cor, fonte e movimento, aparece no manifesto pessoal como `kind: "preset"` e pode ser reutilizada com `{"kind":"library","assetId":"ID-DO-PRESET","cue":0}`. O grupo ou título inserido continua editável no projeto.

Em receitas v2, `{"id":"painel1","kind":"library","assetId":"ID-DO-PRESET","cue":0}` também instancia grupos inteiros. Os filhos mantêm posições locais, duração e keyframes em frames relativos ao cue escolhido (30 FPS); o importador cria novos IDs e preserva a hierarquia. A duração efetiva é limitada pela cena/trecho medido: uma chave que cair depois do fim provoca erro, não um corte silencioso. Se a animação precisa ser ajustada à narração, descreva o grupo diretamente em `children` com keyframes por cue.

Propriedades visuais passadas à referência alteram a raiz; keyframes salvos continuam controlando a animação dessa raiz. Não combine keyframes explícitos com o mesmo preset animado. Se o item do manifesto for uma imagem, seu nome não confirma o conteúdo visual: solicite descrição quando necessário. IDs dependem da instalação e devem ser obtidos exportando o guia atualizado no próprio app.

## Catálogo nativo do aplicativo

)GUIDE";
    for (const auto &asset : nativeLibrary())
        guide += "- `" + QString::fromStdString(asset.id) + "` — " + QString::fromStdString(asset.name) +
                 " (" + QString::fromStdString(asset.category) + "), " + QString::number(asset.width) + "×" +
                 QString::number(asset.height) + ".\n";
    guide += R"GUIDE(

### Direção para roteiro dinâmico

Alterne **texto cinematográfico**, **elementos nativos**, **infográficos ilustrativos**, **travelling entre pontos A/B/C**, **parallax simulado em grupos** e **respiros visuais**. Cada mudança deve revelar informação nova, não apenas movimentar a tela. Use `cameraPresets` ou Visual Beats de câmera para travelling; não combine os dois no mesmo shot. Use `cueProgress` para sincronização sem estimar segundos da fala. Textos grandes devem ter leitura confortável e não disputar espaço com legendas. Cenas com uma frase forte podem ser inteiramente tipográficas. Não mostre gráfico ilustrativo como evidência factual. Para os métodos completos, consulte `docs/DIRECAO-ROTEIRO-DINAMICO.md`.

**Exemplo de elemento nativo:** `{"id":"chip-ia","kind":"native","asset":"chip","x":420,"y":170,"width":320,"height":240}`. Para fundos, use `night-city`, `server-room`, `newsroom` ou outros IDs do catálogo, dimensionados ao canvas. Os novos elementos são vetoriais; use `image`/`video` e fontes locais para fotografias e B-roll reais.

`musicGain`: 0 a 0.08; zero desliga a trilha procedural. `effectsGain`: 0 a 0.15; zero desliga sinais de troca de cena. `sources` pode registrar título e URL HTTPS; é referência editorial, não verificação factual. Revise fatos, fontes e direitos de mídia antes de publicar.

A receita v1 antiga continua sendo lida, com cabeçalho/cenário/layout legado. Receitas v2 mantêm grupos, presets, câmera e keyframes; v3 acrescenta shots, beats, direção de arte por escopo, camadas de vídeo, fades, automação de áudio e animação de legenda por palavra. Campos desconhecidos, IDs duplicados, itens pessoais inativos, arquivos ausentes e valores fora da faixa são rejeitados com mensagem. Projetos v1–v15 continuam abrindo; camadas de vídeo usam schema 5, fades de pista não nulos usam schema 6, legendas com destaque sincronizado por palavra usam schema 7, automação de ganho usa schema 8, efeitos de Visual Beat usam schema 9, controles de fonte de vídeo usam schema 10, estilos por cena/shot usam schema 11, presets de animação por palavra usam schema 12, trim/velocidade da fonte de áudio usa schema 13, cor independente do destaque de palavra usa schema 14 e políticas de texto premium e proveniência dos alvos de `camera.path` usam schema 15.

## Checklist de limites e publicação

O app não baixa mídias da Internet nem gera automaticamente imagens ou vídeos a partir de um prompt. Caminhos relativos de `source` são relativos ao arquivo `.production.json`, em UTF-8. `sources` com URLs HTTPS guarda **referências editoriais**, não verifica fatos nem cria citações visuais. Confira direitos de uso e dados antes de publicar.

Vídeo local como camada com trim de entrada, velocidade, recorte manual, cache de quadros limitado e seek temporal, áudio de vídeo local em pista editável com trim/velocidade, direção de arte global/cena/shot, sincronização por palavra em shots/beats/elementos/keyframes e destaque palavra a palavra nas legendas, seleção múltipla com movimento conjunto na timeline, biblioteca vetorial, áudio em várias pistas, ducking, fades, automação editável graficamente, efeitos sonoros disparados por Visual Beat e telas 1280×720, 1920×1080 ou 1080×1920 estão implementados. Continuam pendentes documentos/PDF com regiões, conectores ancorados, trajetórias Bézier personalizadas, presets visuais nomeados e outras ferramentas avançadas de timeline. Não descreva essas funções pendentes como disponíveis no JSON.

## Capacidades em JSON

```json
)GUIDE";
    guide += QString::fromUtf8(QJsonDocument(capabilities()).toJson());
    guide += "```\n";
    return guide;
}
} // namespace studio
