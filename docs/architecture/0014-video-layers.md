# ADR 0014: vídeo como camada visual

## Estado

Implementada em camadas v3, com trim/velocidade/recorte em projeto schema 10.

## Decisão

Vídeo é um tipo de nó visual do projeto, identificado como `video`, com o mesmo intervalo, hierarquia e transformações dos outros nós. O compositor compartilhado recebe um decodificador FFmpeg por camada; ele entrega quadros RGBA à composição sob demanda, mantendo a implementação compatível com prévia e render-worker.

Receitas v3 podem referenciar mídia local com `kind: "video"` e `source`. Caminhos relativos são resolvidos a partir do JSON. Não se baixa mídia remota. Camadas simples continuam usando schema 5; projetos com trim/velocidade/recorte usam schema 10, enquanto arquivos anteriores mantêm seus schemas e defaults.

O quadro decodificado é ajustado para preencher a caixa do nó, preservando proporção e cortando o excesso no centro. Um objeto `video` da receita pode definir `sourceInSeconds` (0–3600 s), `durationFrames` (1–18000 quadros de timeline), `speed` (0.25–4) e `crop` com coordenadas e dimensões normalizadas dentro da fonte. A duração do nó representa a saída na timeline; a velocidade determina quanto do arquivo fonte é percorrido nesse intervalo.

## Consequências

- Prévia, still export e render MP4 usam o mesmo código de composição.
- O projeto armazena caminho para o vídeo, não uma cópia da mídia. A origem precisa continuar acessível na instalação que abrir/renderizar o projeto.
- O FFmpeg configurado pelo app é necessário para decodificar os formatos de vídeo suportados.
- O áudio contido no arquivo não é importado na faixa visual; deve ser adicionado como pista de áudio independente.
- Ainda não há waveform da fonte, transcrição de palavra sobre vídeo nem sincronização automática do áudio embutido.
- Ao buscar para trás, o decodificador reinicia no início do clipe e percorre os quadros até o ponto solicitado; futuros ajustes devem introduzir busca indexada quando for necessário acelerar scrubbing.

## Verificação

Os testes automatizados geram um clipe local sintético, verificam persistência em schemas 5 e 10, comparam a saída de trim/velocidade com os quadros fonte esperados e garantem que o recorte altere a composição. `scripts/smoke-production.ps1` exporta vídeo com os três controles e compara quadros do MP4 com a prévia; `scripts/smoke.ps1` cobre inicialização, exportação base e recusa de sobrescrita.
