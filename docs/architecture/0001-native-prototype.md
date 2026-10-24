# ADR 0001 — protótipo nativo C++ / Qt Quick

Data: 24/10/2026. Decisão: adotada para o protótipo; desempenho em episódios longos ainda não validado.

> Registro do corte 0.1. A evolução vigente de biblioteca, áudio e produção por receita está em [ADR 0002](0002-ai-directed-production.md). Os limites históricos abaixo não descrevem sozinhos a versão 0.2.

## Contexto e escolha

O usuário escolheu explorar C++ após discutir uma base desktop para um editor audiovisual novo. Esta decisão substitui os candidatos TypeScript/React/Tauri do plano inicial. O repositório anterior foi apenas referência; nenhum código dele foi importado.

Usamos C++20, Qt Quick/QML 6.8.3, CMake 3.31.6 e MinGW 13.1.0. O núcleo usa apenas a biblioteca padrão de C++. A camada de armazenamento depende de Qt Core. A composição usa QPainter/Qt Gui, compartilhada pela prévia e pelo renderizador. Não estamos construindo neste corte um motor GPU ou adotando MLT.

```text
apps/desktop/qml       painéis e interação visual
          ↓
EditorController      adaptação QObject, reprodução, seleção e casos de uso
          ↓
modules/core          documento, tempo racional, avaliação e transações
       ↙        ↘
storage          composition
JSON/revisões    composição QPainter determinística
                       ↑
apps/render-worker     frames → FFmpeg em subprocesso → MP4
```

`core` não depende de Qt. `storage` e `composition` não dependem da interface. `desktop` reúne as dependências. O worker não carrega QML. A interface dispara um snapshot imutável para renderizar; alterações posteriores não mudam uma exportação em andamento.

## Documento e comandos

Schema v1 com projeto, resolução, FPS racional, duração e objetos com IDs. Limites explícitos para este corte: até 200 objetos, 18.000 frames e 3840×2160. Interface cria sequências de 12 segundos em 1280×720 a 30 FPS. O núcleo aceita outras configurações válidas, ainda sem seletor de formato na UI.

Uma alteração é executada em uma cópia candidata, validada e então aplicada. Gestos acumulam uma única entrada no histórico de até 100 snapshots. A API C++ usa callbacks locais; comandos serializáveis para automação externa ainda serão implementados. Não declarar integração com IA pronta.

Modelo `QAbstractListModel` mantém identidade das linhas durante arraste. Seleção e reprodução ficam fora do documento. A animação inicial é um comportamento de entrada de 24 frames; editor de keyframes ainda não existe.

## Persistência

`QSaveFile` grava atomicamente. `QLockFile` coordena escritores cooperativos; SHA-256 detecta revisões obsoletas. Antes de substituir um arquivo conhecido, gravamos `.previous`. Escritores externos que ignorarem o protocolo de lock ainda podem disputar a gravação; isto não constitui um sistema de edição colaborativa.

Autosave é uma cópia de recuperação em `QStandardPaths::AppLocalDataLocation`, independente do arquivo salvo pelo usuário. Não reabrimos nem aplicamos recuperação silenciosamente. Antes da primeira alteração de uma nova sessão, a recuperação anterior permanece disponível. Neste corte existe uma cópia de recuperação compartilhada por instalação: usar uma única janela do editor por vez. Várias instâncias detectam conflito, mas não oferecem recuperação separada por projeto.

Imagens continuam referenciadas por caminho absoluto. Não há catálogo de assets, empacotamento portátil nem reconexão de mídia. A exportação recusa imagens ausentes; a prévia mostra um aviso no espaço correspondente.

## Render e limitações

`PreviewItem` e `studio_render` usam `CompositionPainter` e a mesma avaliação temporal. A prévia inclui uma borda de seleção que não integra o MP4. O backend de prévia neste protótipo é QQuickPaintedItem; uso de Qt Quick não implica renderização própria das composições pela GPU.

FFmpeg recebe RGBA pelo stdin e codifica H.264/yuv420p. Exportação publica o resultado apenas após êxito; arquivo existente é recusado. Cancelamento usa sinalização por arquivo, encerra o FFmpeg e remove o temporário. Não há áudio, vídeo de origem, fila persistente, retomada de render, edição por ripple ou aceleração por hardware neste corte.

## Ferramentas e fontes

Downloads ficam em `.tools/`, ignorada pelo Git. O projeto usa ligação dinâmica com Qt. O preparo é voltado ao desenvolvimento local; empacotamento público, avisos de licença e obrigações de redistribuição precisam de uma etapa própria antes da distribuição.

- [Qt: aplicações QML com CMake](https://doc.qt.io/qt-6/cmake-build-qml-application.html).
- [Qt: QQuickPaintedItem](https://doc.qt.io/qt-6/qquickpainteditem.html).
- [Qt: QSaveFile](https://doc.qt.io/qt-6/qsavefile.html).
- [aqtinstall: instalação das ferramentas Qt](https://aqtinstall.readthedocs.io/en/v3.3.0/cli.html).
- [imageio-ffmpeg: distribuição do executável FFmpeg](https://github.com/imageio/imageio-ffmpeg).

## Próxima fatia recomendada

Catálogo de assets com IDs, importação de vídeo/áudio, relógio audiovisual e sincronização. Avaliar FFmpeg/MLT para essa fatia antes de aumentar os efeitos. Depois: trim por alças, keyframes reais, proxies, métricas e empacotamento.
