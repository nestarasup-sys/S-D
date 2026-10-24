# ADR 0002 — roteiro externo e produção dirigida por JSON

Data: 24/10/2026. Estado: implementado no protótipo 0.2, validado com o episódio curto de carros.

## Objetivo

O usuário escreve um roteiro fora do app e fornece à IA um guia das capacidades existentes. A IA dirige o vídeo em um formato declarativo. O aplicativo valida a receita, gera a voz real, calcula os tempos, monta o projeto editável e exporta MP4.

Não precisamos de chat interno para esse fluxo. O JSON não executa código e não escolhe funcionalidades inexistentes. Guia e catálogo são gerados a partir do registro C++ da biblioteca.

## Estrutura

```text
Roteiro + docs/GUIA-PARA-IA.md
                ↓ IA externa
studio-dark-production v1
                ↓ validação nativa
apps/production-worker → voz SAPI local, trechos WAV
                ↓ leitura PCM e medição real
modules/production → cenas, elementos, legendas, SRT
                ↓
Projeto v2 + narração + mix WAV + receita original + relatório
                ↓
Editor Qt Quick / prévia Qt Multimedia → render worker → H.264 + AAC
```

- `modules/core/library`: IDs dos 14 assets, seis entradas de movimento e quatro poses. Sem Qt.
- `modules/composition/native_painter`: objetos e cenários vetoriais autorais, desenhados com primitivas, curvas e grupos de desenho. Não são PNGs achatados.
- `modules/media`: PCM mono 24 kHz/16 bits, leitura/gravação WAV, trilha procedural e sinais de transição.
- `modules/production`: parsing estrito da receita, catálogo, guia e compilação a partir de amostras de áudio.
- `apps/production-worker`: orquestra voz do Windows e geração de arquivos. O script SAPI é embutido no executável.
- `apps/desktop/production_actions`: acompanha o processo e abre o resultado somente se a edição atual não mudou durante a produção.

## Contrato temporal

Receitas v1 geram sequência 1280×720/30 FPS. Cada cena tem trechos de narração separados. A duração é medida pelo número real de amostras sintetizadas. O compilador insere pausas curtas e arredonda o fim da cena para um frame inteiro (800 amostras a 24 kHz). O domínio geral continua aceitando FPS racional em projetos.

`cue` vincula a entrada de um elemento ao começo de um trecho real. Não é alinhamento de palavras ou fonemas. Legendas são objetos de texto com papel `caption`, editáveis no mesmo inspetor; a mudança do texto não regenera a voz automaticamente.

Cenários, cabeçalhos, objetos e legendas pertencem a cenas identificadas. O modelo da biblioteca/timeline mostra os objetos da cena em que o cursor está, preservando o documento inteiro na exportação. Objetos vinculados não podem ultrapassar o intervalo de sua cena. O áudio master não se desloca ao mover um visual.

## Biblioteca e formatos

Carro, personagem, casa, prédio, documento, computador, celular, moedas, etiqueta e barras ilustrativas; cenários rua, escritório, concessionária e estúdio. Cada objeto aceita transformação, cor e animação. Personagem tem roupa, pele e poses neutral/point/wave/worried. Partes são articuladas pelo desenho parametrizado das poses; ainda não há edição individual dos ossos, caminhada ou sincronização labial.

PNG/JPEG/WebP/BMP continuam como imagens. SVG é importado com Qt SVG e permanece um objeto; não prometemos converter qualquer SVG em partes nativas. Nenhuma mídia remota é baixada pelo plano de produção.

Movimentos de entrada: none, fade, rise, slide-left, slide-right, pop. Deslocamento e giro podem ocorrer ao longo de todo o intervalo. Não há câmera global ou editor arbitrário de keyframes neste corte. As barras são conceituais, não um componente de gráficos numéricos.

## Áudio

Voz pt-BR realmente instalada no Windows; `auto` seleciona uma disponível. SAPI roda no Windows PowerShell, cuja lista de vozes pode diferir da exibida no PowerShell 7. O relatório registra a voz efetivamente usada. Falha de voz não produz um projeto completo silencioso.

A produção preserva WAVs de cada trecho, narração limpa e master com música/efeitos procedurais. Neste corte o editor reproduz e exporta o master; ainda não oferece pistas independentes de mixagem. Alterar volumes de produção ou texto falado requer recompilar a receita. A forma de onda apresentada é medida do master.

Na prévia, o relógio visual acompanha a posição do QMediaPlayer quando há áudio. Navegar pausa a reprodução e a retomada usa o frame selecionado. Na exportação, FFmpeg codifica a mesma composição visual e inclui AAC, com áudio preenchido até a duração do projeto.

## Persistência e falhas

Projeto v2 adiciona cenas, áudio e propriedades nativas. Projetos v1 são migrados em memória com defaults; só são gravados após ação de salvamento, preservando o protocolo de revisão e `.previous`.

Cada produção usa uma pasta nova. Durante geração, o projeto aberto permanece íntegro; se ele mudar, o resultado fica disponível separadamente. Cancelamento da produção é consultado entre trechos SAPI e antes da montagem. O fechamento da janela é bloqueado durante uma tarefa ativa, oferecendo orientação para aguardar ou cancelar.

## Verificação e próximos passos

Rodar `scripts/build.ps1 -Test`, `scripts/smoke.ps1` e `scripts/smoke-production.ps1`. Este último usa a voz real do Windows, verifica PCM/AAC/frames, compara seis frames, testa reprodução audiovisual da prévia e gera pranchas do episódio e da biblioteca.

Ainda faltam avaliação humana de som/ritmo e conforto de edição, integração com novos fornecedores de voz, mixagem em pistas, grafo de objetos com hierarquia editável, importação de vídeo, gráficos factuais, keyframes e câmera. A biblioteca pequena prova o fluxo; não representa um estúdio profissional finalizado.

Referências técnicas: [Qt QMediaPlayer](https://doc.qt.io/qt-6/qmediaplayer.html), [Qt SVG](https://doc.qt.io/qt-6/qsvgrenderer.html), [System.Speech.Synthesis](https://learn.microsoft.com/en-us/dotnet/api/system.speech.synthesis.speechsynthesizer).
