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
            {"projectVersion", 4},
            {"productionCapabilities",
             QJsonArray{"free-scene-layout", "nested-groups", "transform-keyframes-linear",
                        "camera-2d-keyframes", "keyframe-easing-curves", "personal-library-assets",
                        "reusable-group-presets", "optional-visual-captions", "fixed-duration-timing",
                        "cue-progress-sync", "pitch-preserving-atempo", "independent-shots",
                        "visual-beats-show-hide-transform-camera", "shot-cut-camera"}},
            {"projectCapabilities",
             QJsonArray{"group-hierarchy", "transform-keyframes-linear", "camera-2d-keyframes",
                        "keyframe-easing-curves", "shot-ownership", "visual-beat-markers"}},
            {"width", 1280},
            {"height", 720},
            {"fps", 30},
            {"nativeAssets", items},
            {"motions", motions},
            {"poses", poses},
            {"media", QJsonArray{"png", "jpeg", "webp", "bmp", "svg"}},
            {"maxScenes", 20},
            {"maxShotsPerScene", 64},
            {"maxShots", 256},
            {"maxVisualBeats", 4096},
            {"maxNodes", 200},
            {"maxNarrationCharactersPerCue", 150},
            {"audio", QJsonArray{"windows-pt-BR-tts", "procedural-bed", "scene-chime"}}};
}
QString aiGuide(const QJsonObject &library) {
    QString guide = QString::fromUtf8(R"GUIDE(# Studio Dark — guia para dirigir um vídeo com IA

Catálogo 1 · Receita 3 · Projeto 4 · Native Lab 0.4

Você é o diretor audiovisual. Transforme o roteiro em uma produção editável. Preserve o texto do usuário, salvo pedido explícito de reescrita. Escolha enquadramentos e composições livres usando somente os recursos abaixo. Entregue JSON puro, sem cercas Markdown.

## Fluxo

O usuário escreve o roteiro fora do aplicativo, fornece este guia à IA e salva a resposta em `.production.json`. **Importar produção IA** gera voz local, mede cada fala, resolve os cues, monta o projeto e sincroniza o áudio. A pessoa pode editar cada grupo e elemento no editor e exportar o vídeo. Não invente duração nem timestamps da fala.

Receitas v1 e v2 continuam aceitas por compatibilidade. Para novas produções com cortes dentro da mesma fala, use v3.

## Como planejar a receita

- Responda **somente com JSON válido**, sem comentários, cercas Markdown nem texto de apresentação. Use `format: "studio-dark-production"` e `catalogVersion: 1`; escolha `version: 3` para shots/Visual Beats ou `version: 2` para a composição de cena tradicional.
- Preserve o roteiro recebido, salvo instrução de reescrever. Divida cada cena em trechos faláveis, de no máximo 150 caracteres, e use `cue` como índice (a partir de zero) na narração da **mesma cena**.
- `offsetSeconds` varia de 0 a 10; `cueProgress` usa de 0 a 1 para acompanhar a fala final. Sem duração fixa, a duração natural depende da voz pt-BR e só é conhecida depois de sintetizar o WAV.
- Monte a narrativa com composição livre, `group`, `shape`, `text`, `native`, `image`, `svg`, biblioteca pessoal, câmera e keyframes documentados. Reserve espaço para as legendas se `captions` for `true`.
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

`captions` controla as legendas visuais automáticas (padrão `true`). O arquivo SRT ainda é criado mesmo quando `captions` é `false`.

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

Para sincronizar com fala ajustada, use `{"cue":0,"cueProgress":0.5}` no elemento, keyframe ou câmera. `cueProgress` vai de 0 (início da fala) a 1 (último quadro audível); 0,5 significa a metade temporal da **fala ajustada**, não da cena nem de sua pausa. Não misture `cueProgress` e `offsetSeconds` no mesmo objeto. O `offsetSeconds` de 0 a 10 continua disponível para deslocamentos absolutos. Isso não é alinhamento palavra por palavra.

**Planejamento:** para 90 s, 2700 quadros a 30 FPS. A IA pode planejar mas não conhece a duração de uma voz local ainda não gerada. Evite silêncios excessivos; se o plano não couber, ajuste a narração ou os tempos. O aplicativo não inventa áudio para satisfazer qualquer número.

## Receita v3 — cenas, shots e Visual Beats

Uma cena narrativa pode conter vários shots sem repetir nem interromper a narração. A versão 3 aceita as configurações globais da v2 e substitui `elements` e `camera` da cena por `shots`. Cada cena v3 exige de 1 a 64 shots; a produção aceita até 256 shots e 4096 Visual Beats. O primeiro shot começa junto ao início da cena. O início do shot seguinte encerra o anterior; o último termina com a cena. Assim, os planos cobrem a cena sem lacunas ou sobreposições.

Cada shot exige `id`, `start`, `camera` e `elements`; `name` e `transitionIn` são opcionais. IDs são únicos em toda a receita. `start` usa `cue` e um único campo entre `cueProgress` (0–1), `progress` (alias compatível) ou `offsetSeconds` (0–10). O tempo é resolvido sobre o áudio medido. `transitionIn` atualmente aceita somente `cut`, que troca composição e câmera em um corte seco. Câmera do shot exige `x`, `y`, `zoom` e `rotation`; não há foco por ID nesta versão.

Elementos declarados dentro do shot pertencem apenas a ele e viram faixas visuais editáveis no projeto. Sem referência temporal explícita, começam no início do shot. Os elementos e grupos continuam usando os tipos e propriedades documentados abaixo.

Na timeline do projeto gerado, a faixa superior mostra os shots e os marcadores dos beats. Clique para ir ao início do plano/evento. Arraste a alça verde à esquerda de um shot para mover o corte. Os elementos, beats e keyframes de câmera do plano seguinte acompanham o corte; uma operação de arraste é desfeita em um único passo. A edição manual ainda não oferece transições sobrepostas.

Um `beat` exige `id`, `at` e `action`. `at` usa a mesma referência de fala do início do shot. `show` e `hide` exigem `target` e mudam a opacidade do elemento no instante indicado. `transform` exige `target` e ao menos um de `x`, `y`, `rotation`, `scale` ou `opacity`; `durationFrames` opcional (0–300) controla a duração da mudança. `camera` altera `x`, `y`, `rotation` e/ou `zoom`, pode animar por `durationFrames` e não usa `target`. Todos os alvos devem ser elementos daquele shot; beats fora do intervalo são rejeitados. Beats de câmera e transformações são compilados como keyframes para manter o projeto editável.

Exemplo curto, válido e coberto por teste automatizado:

```json
{
  "format": "studio-dark-production",
  "version": 3,
  "catalogVersion": 1,
  "title": "O preço do carro",
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
        "transitionIn": "cut",
        "camera": {"x": 640, "y": 360, "zoom": 1.15, "rotation": 0},
        "elements": [{"id": "preco", "kind": "text", "text": "BARATO?",
          "x": 420, "y": 240, "width": 440, "height": 100, "fontSize": 68,
          "cue": 0, "cueProgress": 0.55}],
        "beats": [
          {"id": "revelar", "at": {"cue": 0, "cueProgress": 0.7},
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

## Composição, grupos e câmera

Não há cabeçalho, título de cena ou cenário forçado em v2. `title`, `kicker` e `background` são opcionais por cena. Sem fundo nativo, o compositor mantém seu gradiente escuro base. Posicione texto e objetos onde fizer sentido; legendas automáticas ocupam a faixa inferior.

`group` exige `children` e pode aninhar até 8 níveis. Coordenadas dos filhos são locais ao canto superior esquerdo do grupo pai. A ordem da lista determina a ordem de desenho. Um grupo pode ter `x`, `y`, `width`, `height`, `rotation`, `scale`, `opacity`, `cue`, `offsetSeconds` e `span` como outro elemento.

`keyframes` em elementos são estados de `x`, `y`, `rotation`, `scale` e `opacity`. Cada chave usa `cue` e `offsetSeconds`; propriedades omitidas mantêm o valor inicial daquele elemento. Opcionalmente, `easing` na **chave de destino** escolhe a interpolação até ela: `linear` (padrão), `ease-in`, `ease-out` ou `ease-in-out`. A primeira chave não tem trecho anterior. As chaves devem cair dentro do trecho de vida do elemento. Evite combinar `keyframes` com `motion`, `travelX`, `travelY` ou `spinDegrees` no mesmo item.

`camera` é uma lista de keyframes na cena com `cue`, `x`, `y`, `zoom` e `rotation`; `offsetSeconds` e `easing` são opcionais. A curva tem a mesma semântica da chave de destino usada nos elementos. O centro inicial é 640×360, zoom 1 e rotação 0. A câmera atravessa os limites entre cenas e mantém o último estado até a próxima chave. Zoom válido: 0.1 a 10. `cue` começa em zero em cada cena.

Para sincronizar, informe `cue` e opcionalmente `offsetSeconds` de 0 a 10. O compilador mede o áudio real. `span` aceita `scene` (padrão) ou `utterance`. Não use timestamps inventados.

## Elementos

`kind`: `group`, `library`, `native`, `text`, `shape`, `image` ou `svg`. `native` usa o campo `asset`; texto usa `text`; mídia direta usa `source`, caminho local absoluto ou relativo ao JSON. Arquivos remotos e URLs não são baixados. PNG com transparência e SVG compatível com Qt são aceitos. Elementos nativos atuais são objetos vetoriais parametrizados.

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

A receita v1 antiga continua sendo lida, com cabeçalho/cenário/layout legado. Receitas v2 mantêm grupos, presets, câmera e keyframes; v3 acrescenta shots e beats. Campos desconhecidos, IDs duplicados, itens pessoais inativos, arquivos ausentes e limites fora da faixa são rejeitados com mensagem. Projetos v1–v3 continuam abrindo; produções v3 são salvas em schema 4 com marcadores editáveis de shots e beats.

## Checklist de limites e publicação

O app não baixa mídias da Internet nem gera automaticamente imagens ou vídeos a partir de um prompt. Caminhos relativos de `source` são relativos ao arquivo `.production.json`, em UTF-8. `sources` com URLs HTTPS guarda **referências editoriais**, não verifica fatos nem cria citações visuais. Confira direitos de uso e dados antes de publicar.

Ainda não existem vídeo como camada, documentos/PDF com regiões, conectores ancorados, foco de câmera por ID, eventos por palavra, trajetórias/curvas Bézier personalizadas, transições além de corte seco, efeitos sonoros disparados por beat, trilhas importadas ou pistas de áudio editáveis. Narração e legendas visuais automáticas usam tempos medidos por fala, não alinhamento palavra a palavra. Não descreva essas funções como disponíveis no JSON.

## Capacidades em JSON

```json
)GUIDE";
    guide += QString::fromUtf8(QJsonDocument(capabilities()).toJson());
    guide += "```\n";
    return guide;
}
} // namespace studio
