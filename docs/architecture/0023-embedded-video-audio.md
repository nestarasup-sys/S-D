# ADR 0023 — áudio embutido em vídeos locais

- Estado: Aceita
- Data: 2026-10-25

## Contexto

Camadas de vídeo locais já permitem trim de fonte, velocidade e duração na timeline, mas o áudio do arquivo não entrava na mixagem. Manter vídeo e áudio como faixas independentes preserva o caráter editável do projeto e permite usar as ferramentas de volume, fade, ducking e automação existentes.

## Decisão

- A ação **Adicionar áudio do vídeo à timeline** verifica se a fonte local contém uma faixa de áudio reconhecida pelo FFmpeg e cria uma `AudioTrack` que aponta para o mesmo arquivo.
- A faixa copia `start`, `duration`, `videoSourceInSeconds` e `videoPlaybackRate` da camada e guarda o ID da camada de vídeo vinculada. Edições posteriores da camada sincronizam a faixa como parte do mesmo comando/undo.
- Mover/aparar a faixa ou editar seu trim/velocidade remove o vínculo; a pista continua no projeto com sua última configuração e passa a ser independente. Excluir a camada também desassocia a pista sem remover o áudio.
- O preview usa `QMediaPlayer` com `playbackRate` e posiciona a fonte em `sourceInSeconds + tempoLocal × playbackRate`.
- A exportação seleciona `0:a:0`, aplica `atrim`, encadeia filtros `atempo` para a faixa suportada de 0,25× a 4×, limita/padroniza à duração da pista e então aplica fades, ganho, automação, ducking e mixagem.
- Receitas v3 podem apontar `audioTracks[].source` para um arquivo de vídeo local com áudio e definir `sourceInSeconds` (0–3600, padrão 0) e `playbackRate` (0,25–4, padrão 1). Campos desconhecidos continuam sendo rejeitados.
- Projetos persistem trim, velocidade e vínculo no schema 13. Versões antigas permanecem válidas e mantêm seus defaults (trim 0, velocidade 1).

## Consequências

- A sincronização da camada e da pista é undoável como uma única edição. Uma alteração manual direta na pista deixa claro que o áudio passa a ser controlado separadamente.
- Apenas a primeira faixa de áudio do contêiner é usada por esta integração. Trocar de faixa de idioma ou escolher um stream por índice fica para uma evolução posterior.
- Formatos sem stream de áudio compatível não podem ser adicionados pela ação do inspetor; o usuário recebe uma mensagem e pode continuar usando fontes de áudio independentes.
- FFmpeg e o backend de mídia do Qt precisam conseguir decodificar a fonte local. O limite de 32 pistas de áudio continua valendo.
