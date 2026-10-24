# Verificação do protótipo 0.1

Data: 24/10/2026. Ambiente: Windows x64, Ryzen 5 4600G, aproximadamente 16 GB de RAM. Qt 6.8.3 / MinGW 13.1.0 / Release.

## Evidência obtida

- `scripts/build.ps1 -Test`: compilação e duas suítes CTest aprovadas.
- Núcleo/armazenamento/composição: 28 verificações, incluindo rejeição de comandos inválidos, transação de arraste, desfazer/refazer, limites temporais, FPS racional, JSON, gravação atômica, cópia anterior e revisão obsoleta.
- Aplicação: 22 verificações diretas da API C++, incluindo identidade estável do modelo, edição, seleção, reabertura, autosave, recuperação, avanço da reprodução e pausa ao navegar. Esses testes não são automação de cliques.
- `scripts/smoke.ps1`: abertura real da aplicação QML, captura da janela e ausência de erros QML detectados no log.
- MP4 real: 12 segundos, 1280×720, 30 FPS, 360 frames decodificados pelo FFmpeg, H.264, sem áudio.
- Comparação do frame 90 do arquivo decodificado com a composição original: PSNR de aproximadamente 44,50 dB, acima do mínimo de 30 dB definido no smoke. É uma amostra, não uma comparação de todos os frames nem da imagem escalada da interface.
- Worker: recusa de sobrescrita com hash do arquivo preservado; cancelamento sem publicar arquivo final; mídia ausente impede exportação.
- Inspeção visual da captura: biblioteca, composição, inspetor e timeline visíveis; texto da demonstração sem corte vertical após ajuste.

Capturas, projeto sintético, MP4 e relatório ficam em `artifacts/smoke-AAAAMMDD-HHMMSS/`. O diretório é local e ignorado pelo Git. O smoke leva aproximadamente cinco segundos neste cenário; esse total inclui inicialização e verificações e não é um benchmark isolado do renderizador.

## Limites da verificação

A ferramenta de controle do computador recusou acesso à janela `studio_dark` por permissão. Não foi possível executar a sequência por cliques e arrastes. A abertura e a captura da aplicação foram verificadas pelo smoke; os casos de uso foram exercitados diretamente pela API C++.

Ainda precisam de avaliação humana: conforto dos gestos, edição em diferentes escalas de tela, comportamento dos diálogos nativos e experiência com uma biblioteca grande. Não houve teste de episódio longo, áudio, vídeo de origem, performance de GPU nem instalação em uma máquina limpa. Essas capacidades não são declaradas prontas.
