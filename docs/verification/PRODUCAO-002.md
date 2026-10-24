# Verificação — primeira produção por roteiro / JSON

Data: 24/10/2026. Windows x64, Ryzen 5 4600G, cerca de 16 GB RAM. Qt 6.8.3, MinGW 13.1.0, Release.

## Resultado

O roteiro **Carro caro: além da etiqueta** foi convertido pela receita versionada em seis cenas, 43 objetos e 1.517 frames. Duração efetivamente medida: 50,57 segundos. Voz usada no Windows PowerShell: **Microsoft Maria Desktop (pt-BR)**. O vídeo foi codificado em H.264/1280×720/30 FPS, com áudio AAC mono a 24 kHz.

## Testes

- Compilação e três suítes CTest: núcleo/armazenamento/composição; aplicação; receita/áudio/biblioteca.
- 57 verificações específicas de produção: parsing com campos opcionais omitidos, IDs e capacidades desconhecidas, valores inválidos, cues, montagem pelo tamanho real dos trechos de áudio, mixagem, WAV, migração e catálogo.
- Smoke visual do editor original aprovado, incluindo cancelamento de render e recusa de sobrescrita/mídia ausente.
- Smoke audiovisual real: voz SAPI → WAVs → compilador → projeto → MP4 → decodificação dos 1.517 frames e áudio.
- WAV master com duração alinhada exatamente aos frames e sinal presente, RMS aproximado de 0,063 e headroom sem saturação na amostra.
- Seis frames do MP4 comparados com a composição de origem: PSNR médio aproximado de 40,67 dB, acima do mínimo de 30 dB. Esse teste é amostral.
- Aplicação abriu o projeto com áudio e avançou do frame 90 para aproximadamente 130 durante a captura automatizada, utilizando o relógio do QMediaPlayer. A captura confirma reprodução em andamento; não é uma escuta humana do som.
- Nome de voz inexistente gera falha, sem publicar projeto completo. Guia e catálogo contêm todos os IDs nativos realmente registrados.
- Pranchas do episódio e dos 14 assets foram geradas pelo motor real e inspecionadas visualmente. A interface recebeu ajustes de contraste nos seletores e espaço reservado para a forma de onda.

## Onde estão as evidências

`artifacts/production-smoke-AAAAMMDD-HHMMSS/` contém `verification.json`, logs, catálogo, prancha da biblioteca e a pasta `production/` com MP4, projeto, roteiro de produção, WAVs, SRT, storyboard e captura da interface. São arquivos locais ignorados pelo Git.

## O que ainda não foi validado

Não houve avaliação humana de pronúncia, trilha e ritmo, nem sequência manual completa de cliques/arraste. A permissão da ferramenta de controle da janela havia sido negada no corte anterior; a verificação desta etapa usa testes C++ e o modo de smoke do próprio aplicativo.

Não houve teste de episódio longo, múltiplas resoluções de tela, instalador limpo, edição colaborativa, vídeo de origem ou mixagem em pistas. As barras do exemplo são conceituais e identificadas assim na imagem; o roteiro não contém preços ou séries estatísticas inventadas.
