# ADR 0006 — presets de grupos reutilizáveis

Data: 24/10/2026. Estado: implementado e verificado localmente.

## Motivação

A biblioteca aceitava um único nó sem keyframes, apesar de projetos schema 3 já suportarem grupos hierárquicos e animações. A IA externa não conseguia reutilizar uma composição inteira salva pelo usuário.

## Decisão

- O comando **Salvar seleção do editor** admite um nó raiz e todos os descendentes, coletados pelos vínculos `parentId`, independentemente da ordem das linhas.
- O preset é armazenado no formato de projeto schema 3, com início normalizado para frame zero, duração da raiz, hierarquia e keyframes relativos. Imagens e SVGs são copiados por hash para a biblioteca; a categoria e o tombstone continuam gerenciados pelo catálogo.
- A reinserção no editor gera IDs novos para cada nó, remapeia os pais, preserva a animação e faz uma única transação de histórico. Se a composição não couber no intervalo restante da cena, falha sem alterar o documento.
- A receita `studio-dark-production` v2 permite `kind: "library"` apontando para um preset composto. O parser expande filhos em memória e os inclui no limite de 200 nós/8 níveis. O compilador resolve o tempo de cada parte e keyframe relativo ao cue medido da voz. Chaves que excedem o trecho resultam em erro explícito.
- Presets legados de nó único sem keyframes continuam sendo estendidos até o final da cena, como antes.

## Limitações

O manifesto pessoal depende da instalação e os assets não são empacotados no JSON. Presets de composição não são instâncias vinculadas nem aceitam retiming automático; não há multi seleção, curvas Bézier personalizadas, vídeo como camada nem componentes com parâmetros editáveis em lote. A animação aceita as curvas nomeadas `linear`, `ease-in`, `ease-out` e `ease-in-out`.

## Verificação

Foram acrescentados testes automatizados para salvamento, reinserção, undo, IDs, hierarquia, keyframes e compilação por JSON. Em 24/10/2026, `scripts/build.ps1 -Test` passou (4/4), assim como `scripts/smoke.ps1`, `scripts/smoke-workspace.ps1`, `scripts/smoke-production.ps1` e `scripts/update-ai-guide.ps1`. O smoke de produção usa a voz SAPI instalada `Microsoft Maria Desktop` e compara quadros do MP4 renderizado com a composição.

## Pacotes premium

O pacote premium não substitui o preset composto: templates registrados instanciam nós com o mesmo formato de projeto e comando de remapeamento de IDs. O registro C++ possui construtor, ID, tipo, categoria e referência ao manifesto v1. Manifests são recursos Qt embutidos e usados para gerar o registro do Workspace; na inicialização, o aplicativo confere ID/tipo/nome, contagem da árvore, IDs e tipos de camada, parent IDs e cada papel de slot com o resultado real do construtor. Os três templates oficiais são smartphone, Title Reveal e Comparison Pack. Itens são adicionados apenas quando seus IDs não existem; catálogo pessoal, lixeira e categorias permanecem intocados.

O validador premium v1 rejeita IDs de camada duplicados, slots sem camada, tipos desconhecidos, limites acima de 200 camadas/64 slots, dependências repetidas e caminhos absolutos ou que escapem da raiz do pacote. Os manifestos embutidos são comparados com as árvores criadas pelos construtores oficiais; eles descrevem, mas não executam o desenho. A edição resultante é independente do manifesto e usa nós normais do projeto. A instalação de pacotes externos ainda não existe. Os slots premium são somente texto/imagem; imagens escolhidas no inspetor são decodificadas, limitadas a 32 MiB/40 MP e copiadas por SHA-256 para `workspace/library/media`. Selecionar novamente conteúdo idêntico reutiliza o mesmo arquivo; desfazer não remove mídia referenciada nem faz limpeza física prematura. Projetos antigos que usam caminhos externos continuam abrindo; se uma referência faltar, a renderização mostra o aviso de mídia ausente e o inspetor permite substituir um slot premium. Os caminhos salvos continuam vinculados ao workspace local, então essa cópia, isoladamente, não torna o projeto portátil entre computadores.

O registro premium declara layouts específicos para 1280×720, 1920×1080 e 1080×1920. A receita v3 chama o mesmo construtor com dimensões escolhidas; a inserção no editor também usa o canvas do projeto. O Title Reveal usa um papel de slot verificado e um wipe QPainter com clip progressivo; texto premium reduz o tamanho da fonte até caber no bounds antes de desenhar. Durante a compilação da receita, somente os pacotes oficiais registrados ajustam seus keyframes, motionFrames e saída ao intervalo real medido da cena/shot. Presets pessoais conservam offsets/tempos e continuam sujeitos à validação que rejeita keyframes fora da fala. Não há novos campos JSON para parametrizar slots: a edição acontece depois da inserção e continua sendo edição padrão de nós/keyframes.
