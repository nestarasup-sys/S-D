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
            {"productionVersion", 2},
            {"projectVersion", 3},
            {"productionCapabilities",
             QJsonArray{"free-scene-layout", "nested-groups", "transform-keyframes-linear",
                        "camera-2d-keyframes", "keyframe-easing-curves", "personal-library-assets",
                        "reusable-group-presets", "optional-visual-captions"}},
            {"projectCapabilities",
             QJsonArray{"group-hierarchy", "transform-keyframes-linear", "camera-2d-keyframes",
                        "keyframe-easing-curves"}},
            {"width", 1280},
            {"height", 720},
            {"fps", 30},
            {"nativeAssets", items},
            {"motions", motions},
            {"poses", poses},
            {"media", QJsonArray{"png", "jpeg", "webp", "bmp", "svg"}},
            {"maxScenes", 20},
            {"maxNodes", 200},
            {"maxNarrationCharactersPerCue", 150},
            {"audio", QJsonArray{"windows-pt-BR-tts", "procedural-bed", "scene-chime"}}};
}
QString aiGuide(const QJsonObject &library) {
    QString guide = QString::fromUtf8(R"GUIDE(# Studio Dark — guia para dirigir um vídeo com IA

Catálogo 1 · Receita 2 · Projeto 3 · Native Lab 0.4

Você é o diretor audiovisual. Transforme o roteiro em uma produção editável. Preserve o texto do usuário, salvo pedido explícito de reescrita. Escolha enquadramentos e composições livres usando somente os recursos abaixo. Entregue JSON puro, sem cercas Markdown.

## Fluxo

O usuário escreve o roteiro fora do aplicativo, fornece este guia à IA e salva a resposta em `.production.json`. **Importar produção IA** gera voz local, mede cada fala, resolve os cues, monta o projeto e sincroniza o áudio. A pessoa pode editar cada grupo e elemento no editor e exportar o vídeo. Não invente duração nem timestamps da fala.

Receitas v1 continuam aceitas por compatibilidade. Para novas produções, use v2.

## Como planejar a receita

- Responda **somente com JSON válido**, sem comentários, cercas Markdown nem texto de apresentação. Use exatamente `format: "studio-dark-production"`, `version: 2` e `catalogVersion: 1`.
- Preserve o roteiro recebido, salvo instrução de reescrever. Divida cada cena em trechos faláveis, de no máximo 150 caracteres, e use `cue` como índice (a partir de zero) na narração da **mesma cena**.
- `offsetSeconds` varia de 0 a 10; não é timestamp absoluto. A duração real depende da voz pt-BR instalada e só é conhecida depois de sintetizar o WAV.
- Monte a narrativa com composição livre, `group`, `shape`, `text`, `native`, `image`, `svg`, biblioteca pessoal, câmera e keyframes documentados. Reserve espaço para as legendas se `captions` for `true`.
- Conte no máximo 200 nós, incluindo filhos, legendas automáticas, título/kicker e cenário. Evite elementos ilegíveis em 1280×720; respeite o limite de oito níveis de grupo.
- Nunca invente `assetId`, caminho de mídia, arquivo de vídeo, fonte jornalística ou valor numérico como se fosse real. Se faltar mídia, componha com formas e textos ou indique ao usuário o arquivo necessário.
- Revise antes de responder: JSON parseável, IDs únicos, no máximo 20 cenas, de 1 a 10 falas por cena, propriedades dentro dos limites e narração coerente com os visuais.

## Receita v2

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

## Composição, grupos e câmera

Não há cabeçalho, título de cena ou cenário forçado em v2. `title`, `kicker` e `background` são opcionais por cena. Sem fundo nativo, o compositor mantém seu gradiente escuro base. Posicione texto e objetos onde fizer sentido; legendas automáticas ocupam a faixa inferior.

`group` exige `children` e pode aninhar até 8 níveis. Coordenadas dos filhos são locais ao canto superior esquerdo do grupo pai. A ordem da lista determina a ordem de desenho. Um grupo pode ter `x`, `y`, `width`, `height`, `rotation`, `scale`, `opacity`, `cue`, `offsetSeconds` e `span` como outro elemento.

`keyframes` em elementos são estados de `x`, `y`, `rotation`, `scale` e `opacity`. Cada chave usa `cue` e `offsetSeconds`; propriedades omitidas mantêm o valor inicial daquele elemento. Opcionalmente, `easing` na **chave de destino** escolhe a interpolação até ela: `linear` (padrão), `ease-in`, `ease-out` ou `ease-in-out`. A primeira chave não tem trecho anterior. As chaves devem cair dentro do trecho de vida do elemento. Evite combinar `keyframes` com `motion`, `travelX`, `travelY` ou `spinDegrees` no mesmo item.

`camera` é uma lista de keyframes na cena com `cue`, `x`, `y`, `zoom` e `rotation`; `offsetSeconds` e `easing` são opcionais. A curva tem a mesma semântica da chave de destino usada nos elementos. O centro inicial é 640×360, zoom 1 e rotação 0. A câmera atravessa os limites entre cenas e mantém o último estado até a próxima chave. Zoom válido: 0.1 a 10. `cue` começa em zero em cada cena.

Para sincronizar, informe `cue` e opcionalmente `offsetSeconds` de 0 a 10. O compilador mede o áudio real. `span` aceita `scene` (padrão) ou `utterance`. Não use timestamps inventados.

## Elementos

`kind`: `group`, `library`, `native`, `text`, `shape`, `image` ou `svg`. `native` usa o campo `asset`; texto usa `text`; mídia direta usa `source`, caminho local absoluto ou relativo ao JSON. Arquivos remotos e URLs não são baixados. PNG com transparência e SVG compatível com Qt são aceitos. Elementos nativos atuais são objetos vetoriais parametrizados.

Propriedades: `x`, `y`, `width`, `height`, `color` (#RRGGBB), `secondaryColor`, `fontSize` (8–300), `opacity` (0–1), `rotation`, `scale` (0.01–10), `travelX`, `travelY`, `spinDegrees`, `motion`, `motionFrames` (1–300), `exitFrames` (0–300), `pose`, `cue`, `offsetSeconds`, `span`, `keyframes`, `children`, `assetId`.

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

A receita v1 antiga continua sendo lida, com cabeçalho/cenário/layout legado. No formato v2, campos desconhecidos, IDs duplicados, itens pessoais inativos, arquivos ausentes e limites fora da faixa são rejeitados com mensagem. O projeto compilado permanece editável em schema 3.

## Checklist de limites e publicação

O app não baixa mídias da Internet nem gera automaticamente imagens ou vídeos a partir de um prompt. Caminhos relativos de `source` são relativos ao arquivo `.production.json`, em UTF-8. `sources` com URLs HTTPS guarda **referências editoriais**, não verifica fatos nem cria citações visuais. Confira direitos de uso e dados antes de publicar.

Ainda não existem vídeo como camada, documentos/PDF com regiões, conectores ancorados, foco de câmera por ID, eventos por palavra, curvas de easing ou pistas de áudio editáveis. Narração e legendas visuais automáticas usam tempos medidos por fala, não alinhamento palavra a palavra. Não descreva essas funções como disponíveis no JSON.

## Capacidades em JSON

```json
)GUIDE";
    guide += QString::fromUtf8(QJsonDocument(capabilities()).toJson());
    guide += "```\n";
    return guide;
}
} // namespace studio
