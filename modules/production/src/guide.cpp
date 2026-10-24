#include <QJsonArray>
#include <QJsonDocument>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>

namespace studio {
QJsonObject capabilities() {
    QJsonArray items, motions, poses;
    for (const auto &a : nativeLibrary())
        items.append(QJsonObject{{"id", QString::fromStdString(a.id)},
                                 {"name", QString::fromStdString(a.name)},
                                 {"category", QString::fromStdString(a.category)},
                                 {"width", a.width},
                                 {"height", a.height}});
    for (const auto &v : motionNames())
        motions.append(QString::fromStdString(v));
    for (const auto &v : poseNames())
        poses.append(QString::fromStdString(v));
    return {{"format", "studio-dark-capabilities"},
            {"catalogVersion", 1},
            {"productionVersion", 1},
            {"projectVersion", 3},
            {"projectCapabilities",
             QJsonArray{"group-hierarchy", "transform-keyframes-linear", "camera-2d-keyframes"}},
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
QString aiGuide() {
    QString g = QString::fromUtf8(R"GUIDE(# Studio Dark — guia para dirigir um vídeo com IA

Catálogo 1 · Receita 1 · Projeto 3 · Native Lab 0.3

Você é o diretor audiovisual. Transforme o roteiro fornecido pelo usuário em uma receita JSON válida, usando apenas as capacidades abaixo. Preserve o texto do roteiro, salvo pedido explícito de reescrita. Entregue JSON puro, sem comentários ou cercas Markdown.

## Fluxo

O usuário escreve o roteiro fora do aplicativo, fornece este guia à IA e salva a resposta em um arquivo `.production.json`. No aplicativo, **Importar produção IA** gera voz local, mede a duração real de cada trecho, monta cenas/legendas e abre o projeto editável. Exportar MP4 cria o arquivo final com áudio. Não escreva timestamps inventados para a voz.

## Capacidades do projeto editável

Depois de aberto no editor, um projeto schema 3 pode ter grupos hierárquicos, keyframes lineares de posição/escala/rotação/opacidade e uma câmera 2D com keyframes de centro/zoom/rotação. O inspetor permite criar um grupo para o objeto selecionado, registrar keyframes no quadro atual e animar a câmera. Prévia e exportação usam o mesmo avaliador.

Essas capacidades pertencem ao formato do projeto e aos controles manuais atuais. **A receita IA abaixo continua na versão 1 e ainda não consegue descrever grupos, keyframes ou câmera.** Não invente esses campos na receita: o importador rejeita chaves desconhecidas. Para usá-los hoje, importe a receita v1 e complemente a composição no editor.

## Contrato obrigatório

```json
{
  "format": "studio-dark-production",
  "version": 1,
  "catalogVersion": 1,
  "title": "Título do vídeo",
  "voice": "auto",
  "speechRate": 0,
  "musicGain": 0.025,
  "effectsGain": 0.035,
  "sources": [],
  "scenes": [{
    "id": "abertura",
    "title": "Seu título na tela",
    "kicker": "CAPÍTULO 01",
    "background": "street",
    "narration": ["Primeira frase do roteiro.", "Segunda frase do roteiro."],
    "elements": [{
      "id": "carro-abertura",
      "kind": "native",
      "asset": "car",
      "x": 350, "y": 340, "width": 460, "height": 210,
      "color": "#efb873", "motion": "slide-right",
      "cue": 0, "travelX": 60
    }]
  }]
}
```

Sequência desta versão: 1280×720, 30 FPS. Use de 1 a 20 cenas e até 200 objetos, contando cenário, dois cabeçalhos e uma legenda por trecho. Cada cena tem 1 a 10 trechos de narração; cada trecho tem até 150 caracteres. Divida frases compridas em pausas naturais. Título do vídeo: até 120 caracteres; título e kicker de cada cena: até 70. Evite mais de duas linhas no título e confira o resultado visual.

`voice`: `auto` seleciona uma voz pt-BR realmente instalada; também aceita nome exato de voz. `speechRate`: inteiro entre -3 e 3. Sem voz compatível, a produção falha de forma explícita. A voz é sintetizada no Windows, sem serviço externo. A qualidade depende da voz instalada.

`musicGain`: 0 a 0.08; zero desliga a trilha procedural. `effectsGain`: 0 a 0.15; zero desliga os sinais sonoros de troca de cena. O arquivo final mistura esses sons à voz. Os níveis podem ser alterados na receita e recompilados.

## Elementos

`kind` aceita `native`, `text`, `shape`, `image` ou `svg`. `native` exige `asset` do catálogo; `text` usa `text`; imagem/SVG usam `source` com caminho de arquivo local (relativo ao JSON ou absoluto). URLs remotas não são importadas. Arquivos precisam existir na máquina do usuário.

Propriedades opcionais: `x`, `y`, `width`, `height`, `color` (#RRGGBB), `secondaryColor`, `fontSize` (8–300), `opacity` (0–1), `rotation` (graus), `scale` (0.01–10), `travelX`, `travelY`, `spinDegrees`, `motion`, `motionFrames` (1–300), `exitFrames` (0–300), `pose`, `cue`, `offsetSeconds`, `span`.

Coordenadas em pixels da composição. Mantenha ilustrações principalmente entre y=240 e y=595. O título ocupa x=70, y=92, largura 1140, altura 140. Legendas ocupam x=80, y=620, largura 1120, altura 80. Evite elementos importantes nessas áreas reservadas.

`cue` é o índice do trecho de narração na cena, começando em zero. O objeto entra junto do áudio real daquele trecho. `offsetSeconds` atrasa a entrada em até dez segundos, desde que ainda caiba no intervalo. `span: "scene"` mantém o objeto até o fim da cena; `"utterance"` limita ao trecho escolhido. Não use duração absoluta de cena: ela será calculada com o áudio real.

`travelX`/`travelY` deslocam o objeto linearmente durante seu intervalo. `spinDegrees` adiciona rotação ao longo do intervalo. `motion` define a entrada; `exitFrames` define o fade de saída. A receita v1 não contém keyframes arbitrários ou câmera global, embora o projeto 3 permita criá-los manualmente depois da importação.

Personagem: `color` controla a roupa e `secondaryColor` a pele. `pose` escolhe a posição dos braços; `wave` anima a saudação. O personagem é um objeto vetorial parametrizado: as partes não são trilhas separadas e não há sincronização labial ou caminhada automática.

Barras são **ilustrativas**, sem valores numéricos de dados. Não use suas alturas como estatística. Use textos para explicar conceitos; para fatos ou números, registre as fontes e aguarde revisão humana.

SVG usa os recursos suportados pelo Qt SVG; não existe conversão geral de SVG em partes editáveis. PNG com transparência é aceito como imagem. Não há importação de clipe de vídeo, busca de mídia, geração de imagens, 3D ou código executável no plano.

`sources`: lista opcional de objetos com `title` e `url` HTTPS. É registro editorial copiado na receita; o aplicativo não verifica os fatos nem consulta os links automaticamente. Campos desconhecidos são rejeitados. Não invente capacidades, fontes ou alinhamento por palavra.

## Catálogo real do aplicativo

)GUIDE");
    for (const auto &a : nativeLibrary())
        g += "- `" + QString::fromStdString(a.id) + "` — " + QString::fromStdString(a.name) + " (" +
             QString::fromStdString(a.category) + "), " + QString::number(a.width) + "×" +
             QString::number(a.height) + ".\n";
    g += "\nMovimentos:";
    for (const auto &v : motionNames())
        g += " `" + QString::fromStdString(v) + "`";
    g += "\n\nPoses:";
    for (const auto &v : poseNames())
        g += " `" + QString::fromStdString(v) + "`";
    g += "\n\n## Capacidades em JSON\n\n```json\n" +
         QString::fromUtf8(QJsonDocument(capabilities()).toJson()) + "```\n";
    return g;
}
} // namespace studio
