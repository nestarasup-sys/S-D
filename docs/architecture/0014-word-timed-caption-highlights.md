# ADR 0014: Destaque de palavras sincronizado nas legendas

## Status

Implementada em projetos schema 7, com alinhamento obtido da síntese de voz local.

## Decisão

- Cada nó de legenda pode persistir intervalos `CaptionWordCue` com texto e quadros inicial/final.
- O compilador cria esses intervalos somente quando a fala tem eventos de palavra reais e o texto da legenda pode ser alinhado aos eventos. Não estima tempos com base na quantidade de caracteres.
- O compositor compartilha o mesmo desenho de palavras entre prévia e exportação e aplica a cor primária da direção de arte à palavra ativa.
- Se mais de uma palavra cair no mesmo quadro, o compilador reduz o grupo à última palavra iniciada naquele quadro, porque a sequência final não consegue mostrar dois estados distintos dentro de um único quadro.
- Projetos schema 1–6 continuam carregando. Intervalos sincronizados são gravados em schema 7; a ausência desses dados mantém as legendas estáticas antigas.

## Limitações

- A voz pt-BR local precisa ser sintetizada novamente para reconstruir os tempos. O projeto guarda os intervalos finais, mas a receita ainda não guarda um alinhamento fonético editável que sobreviva à troca de voz.
- A cor ativa usa a cor primária global e não tem controle por legenda, palavra ou shot.
- As legendas continuam sendo blocos de texto por fala; não há editor visual de cue, quebra manual ou karaoke com avanço contínuo dentro do fonema.
