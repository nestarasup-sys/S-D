# ADR 0014: vídeo como camada visual

## Estado

Aceito e implementado parcialmente no Studio Dark.

## Decisão

Vídeo é um tipo de nó visual do projeto, identificado como `video`, com o mesmo intervalo, hierarquia e transformações dos outros nós. O compositor compartilhado recebe um decodificador FFmpeg por camada; ele entrega quadros RGBA à composição sob demanda, mantendo a implementação compatível com prévia e render-worker.

Receitas v3 podem referenciar mídia local com `kind: "video"` e `source`. Caminhos relativos são resolvidos a partir do JSON. Não se baixa mídia remota. Projetos com camadas de vídeo usam schema 5, enquanto arquivos anteriores mantêm seus schemas e defaults.

O quadro decodificado é ajustado para preencher a caixa do nó, preservando proporção e cortando o excesso no centro. O tempo de reprodução corresponde ao intervalo do nó e à taxa de quadros da sequência.

## Consequências

- Prévia, still export e render MP4 usam o mesmo código de composição.
- O projeto armazena caminho para o vídeo, não uma cópia da mídia. A origem precisa continuar acessível na instalação que abrir/renderizar o projeto.
- O FFmpeg configurado pelo app é necessário para decodificar os formatos de vídeo suportados.
- O áudio contido no arquivo não é importado na faixa visual; deve ser adicionado como pista de áudio independente.
- Ainda não há controle de ponto de entrada, velocidade, recorte manual, waveform, transcrição de palavra sobre vídeo, nem sincronização automática de áudio embutido.
- Ao buscar para trás, o decodificador reinicia no início do clipe e percorre os quadros até o ponto solicitado; futuros ajustes devem introduzir busca indexada quando for necessário acelerar scrubbing.

## Verificação

O teste automatizado gera um clipe local sintético, verifica sua ida e volta pelo schema 5 e compara quadros em tempos diferentes pelo compositor. Smokes completos de UI e produção são necessários após a rodada.
