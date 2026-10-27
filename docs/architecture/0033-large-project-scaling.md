# ADR 0033 — escala de projetos sem teto agregado de objetos

## Estado

Aceita para a branch de desenvolvimento.

## Contexto

Receitas podiam ser recusadas por um teto histórico de 200 objetos, e o parser de receitas e de projetos também cortava arquivos por tamanho (1 MiB e 4 MiB). A expansão de recursos premium e legendas pode ultrapassar esse total sem que a produção seja malformada.

## Decisão

- Não existe teto agregado de nós na receita nem no projeto. O armazenamento usa `std::size_t`; somas e expansões verificam overflow.
- Não existe teto fixo de bytes para os documentos de receita/projeto. A leitura, o parser e a serialização continuam sujeitos à memória e aos recursos disponíveis. Falhas de alocação retornam diagnóstico de memória insuficiente e gravações continuam atômicas.
- Validação de IDs, referências, ciclos, campos, caminhos, limites de profundidade e limites independentes de duração, cenas, shots, beats, texto, áudio e keyframes continuam ativos. Esses limites não são um substituto para a quantidade de nós.
- O grafo de projeto cria índices temporários por ID para validar referências e detectar ciclos em tempo linear esperado. A compilação também indexa os nós para focos de câmera e alvos de Visual Beats.
- Listas Qt continuam usando `ListView` virtualizada; a resolução da linha de um nó selecionado usa índice por ID. Mudanças de seleção notificam a seleção sem disparar uma atualização completa do documento.
- O QPainter continua sendo o renderizador de referência. A remoção do teto permite processar projetos maiores, mas não promete taxa de quadros em tempo real para cargas arbitrárias.

## Verificação

`tests/large_project_scalability_tests.cpp` executa importação de receitas, expansão dos três templates premium, grupos aninhados, keyframes, shots, Visual Beats, legenda automática, validação, salvar/abrir, snapshot, edição/undo/redo e renderização QPainter para tamanhos progressivos. O maior cenário gera um documento de receita acima de 1 MiB e um projeto acima de 4 MiB.

O artefato `artifacts/large-project-scaling/report.json` é local e ignorado pelo Git. Uma execução QPainter 1280×720 no Ryzen 5 4600G/16 GB mediu:

| Nós efetivos | Parse da receita | Compile | Salvar | Reabrir | Snapshot | Validar | Render de 1 quadro | RAM privada durante a carga |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 201 | 3,84 ms | 13,28 ms | 18,31 ms | 12,58 ms | 0,29 ms | 0,16 ms | 40,07 ms | 13,8 MB |
| 217 | 3,66 ms | 13,43 ms | 15,92 ms | 13,22 ms | 0,25 ms | 0,14 ms | 37,94 ms | 14,3 MB |
| 300 | 5,29 ms | 13,55 ms | 19,58 ms | 15,52 ms | 0,39 ms | 0,19 ms | 52,56 ms | 16,3 MB |
| 500 | 9,01 ms | 13,55 ms | 29,97 ms | 19,32 ms | 0,62 ms | 0,28 ms | 88,52 ms | 19,8 MB |
| 1.000 | 18,56 ms | 14,74 ms | 55,51 ms | 35,85 ms | 1,13 ms | 0,54 ms | 184,89 ms | 27,5 MB |
| 3.000 | 56,99 ms | 19,12 ms | 162,48 ms | 107,13 ms | 3,70 ms | 1,52 ms | 559,35 ms | 62,1 MB |

No maior caso, a receita tinha 3.497.527 bytes (3,34 MiB), o projeto 6.075.908 bytes (5,79 MiB); o working set de pico amostrado foi 77,2 MB decimais. Medições de uma execução e uma fixture, não garantias de orçamento ou taxa de quadros. O custo de edição/undo/redo medido para 3.000 nós foi 9,84 ms. O render de milhares de camadas custa centenas de milissegundos por quadro neste cenário.

O arquivo original `STUDIO-DARK-LABORATORIO-DE-MOVIMENTO.production.json` (66.376 bytes; SHA-256 registrado localmente como `FD61B9BC6898126EB034C87C182035B4C317D3671B141652F56D4AD86C908037`) foi compilado sem alteração e expandiu para 217 nós, 7 cenas, 23 shots, 28 Visual Beats e legenda automática. Microsoft Daniel gerou voz e eventos de sincronização; a receita pedia 73 s, mas a fala medida exigiu 120,17 s com o limite de velocidade de 1,15×, e o relatório avisou sobre expansão dos intervalos. O projeto abriu no editor e o MP4 com áudio foi decodificado por inteiro: 3.605 quadros H.264 1920×1080 a 30 fps e AAC mono. O projeto, o log e o MP4 de validação permanecem em `artifacts/large-production-regression-laboratorio/`, fora do Git.

O protótipo Scene Graph Direct3D11 também foi ensaiado em 201–300 formas. Em 300, a métrica de cor falhou por margem pequena (MAE 3,067, limite vigente 3,000), embora IoU 0,954 e F1 de bordas 1,000 tenham passado. O benchmark encerrou essa matriz sem enfraquecer o limite. A validação de qualidade de cenas maiores no protótipo GPU segue pendente; o editor oficial usa QPainter e passou a matriz estrutural.

A receita original do Laboratório de Movimento também deve ser validada sem alteração; voz e temporização dependem da voz pt-BR instalada, e um MP4 é uma validação de fluxo, não um arquivo para versionar.

## Consequências

Projetos grandes podem exigir mais memória e tempo de importação, edição, salvamento e renderização. A aplicação informa falhas reais de parser, memória e I/O onde detectáveis. O processo de leitura de JSON ainda precisa materializar o documento em memória; portanto, o limite prático depende do Windows, do Qt, do tamanho de cada elemento e da memória disponível. Não há promessa de sucesso para cargas que excedam esses recursos.
