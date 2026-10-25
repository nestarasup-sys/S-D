# Studio Dark · Native Lab

Protótipo 0.3 em C++20 e Qt Quick/QML: projetos e biblioteca organizados, roteiro externo → direção por JSON → voz local → projeto editável → MP4 com áudio.

## Abrir

Clique em **ABRIR-STUDIO.bat**. As ferramentas locais ficam em `.tools/` e os binários em `build/`. O BAT configura os caminhos necessários e abre a versão compilada ativa sem terminal permanente. Não execute apenas o EXE fora desse ambiente de desenvolvimento. Se o editor estiver aberto durante uma compilação, o build usa uma pasta nova e preserva a sessão existente; a próxima abertura usa a versão nova.

Em outra máquina Windows x64 com Python 3.12 e internet, use **PREPARAR-E-COMPILAR.bat**. Downloads não exigem instalação global de Qt ou C++ e ficam na pasta do projeto.

Para gerar o primeiro episódio completo, clique em **CRIAR-VIDEO-CARROS.bat**. Ele cria uma pasta nova em `artifacts/carros-AAAAMMDD-HHMMSS`, gera voz/legendas/cenas, exporta o vídeo e abre o projeto no editor. É necessária uma voz pt-BR instalada no Windows.

## Tela inicial de projetos

O aplicativo abre em **Projetos**, com cartões, prévias, busca e atalhos para a biblioteca. **Novo projeto** pede um nome, cria uma composição vazia e já salva o arquivo na pasta de dados do aplicativo. **Importar projeto** adiciona um arquivo existente à coleção, preservando sua localização.

Cada cartão oferece **Abrir**, **Duplicar**, **Renomear** e **Excluir**. A exclusão move o registro para a **Lixeira**, de onde pode ser restaurado; os arquivos do projeto e suas mídias são mantidos. Projetos gerados anteriormente na pasta de produções do app são reconhecidos na primeira inicialização da coleção.

## Biblioteca completa

O botão **Biblioteca completa** está sempre no topo e também no painel do editor. A tela reúne objetos nativos, cenários, personagens, dados, imagens e variações personalizadas.

- **Adicionar arquivo** importa PNG/JPEG/WebP/BMP ou SVG, com nome e categoria. O app guarda uma cópia própria da mídia.
- **Salvar seleção do editor** transforma o objeto selecionado em um item reutilizável, preservando estilo, pose, tamanho e movimento. Imagens/SVG também têm sua mídia copiada.
- **Organizar** renomeia ou muda a categoria de um item. **Usar** adiciona uma nova instância ao projeto aberto, com identidade independente.
- Categorias podem ser criadas, renomeadas e excluídas. Ao excluir uma categoria, seus itens vão para **Sem categoria**.
- **Excluir** remove o item da coleção; ele pode ser restaurado em **Excluídos / restaurar**. Nativos continuam disponíveis ao motor de renderização e as mídias importadas são retidas, preservando projetos já criados.

As lixeiras são reversíveis e não fazem limpeza física de arquivos. Não há purga permanente neste corte. A organização da biblioteca é local e não altera o catálogo de capacidades nativas do guia de produção.

## Seu roteiro com uma IA externa

1. Escreva seu roteiro em qualquer lugar.
2. Use o botão **Guia IA** para salvar o guia atual com os itens ativos da sua biblioteca pessoal. A cópia do repositório lista as capacidades nativas e explica como salvar um manifesto local.
3. Envie à IA o roteiro e o guia gerado; peça uma receita v3 quando quiser cortes/ações visuais dentro da mesma cena, ou v2 para uma composição tradicional, e salve como `.production.json`.
4. No editor, use **Importar produção IA**. O app valida o plano, gera a voz local, mede os tempos e monta as cenas.
5. Reproduza, ajuste os objetos e exporte MP4.

Receita v2 aceita composições sem títulos ou cenários impostos, grupos, keyframes ligados às falas, câmera e referências por ID a itens pessoais ativos. Receitas v1 antigas continuam funcionando. Exemplos: [roteiro sobre carros](examples/ROTEIRO-CARROS-CAROS.md), [receita v1 de carros](examples/carros-caros.production.json) e [composição livre v2](examples/composicao-livre-v2.production.json).

## Shots e Visual Beats (receita v3)

Uma cena pode ter vários shots sem quebrar a narração. O início de cada plano é resolvido contra a fala sintetizada; o próximo início encerra o plano anterior, e os planos precisam cobrir a cena sem lacunas. Cada shot tem composição e estado inicial de câmera próprios. `camera.focus` pode enquadrar um elemento ou grupo do próprio shot com margem automática; `follow: true` acompanha o alvo quadro a quadro. `transitionIn` aceita corte seco, fade, dissolve, slide horizontal/vertical e zoom com duração configurável em quadros. No editor, selecione um quadro dentro do shot e ajuste tipo, duração e direção da transição de entrada no inspetor; o primeiro shot da cena permanece sem transição.

Receitas v3 também aceitam `artDirection` para definir paleta, fonte instalada, tamanhos tipográficos, posição das legendas e margem segura como estilo global. Esses valores funcionam como padrões para elementos, títulos gerados e legendas; propriedades de elemento explicitamente definidas continuam prevalecendo. Estilos de arte por cena e arquivos de fonte próprios ainda estão pendentes.

No inspetor, **Direção de arte global** edita as cores, a fonte, a tipografia, a margem segura e a posição das legendas. **Intensidade de edição** aplica à cena atual um perfil de entrada/saída entre 0 e 100; a ação inteira pode ser desfeita de uma vez. Ajustes finos continuam disponíveis por objeto.

Receitas v3 aceitam `kind: "video"` com um caminho local em `source` para inserir vídeo/B-roll como camada editável da composição. O vídeo toca na prévia e entra no MP4 exportado, mantém a proporção com corte central e usa a duração do elemento. Formatos aceitos: MP4, MOV, MKV, WebM, AVI e M4V. A máquina que abrir/exportar o projeto precisa ter acesso ao arquivo e ao FFmpeg configurado pelo app. O projeto sobe para schema 5 quando contém vídeo; schemas anteriores continuam compatíveis.

Projetos schema 4–6 e receitas v3 aceitam até 32 pistas de áudio independentes. Na timeline, importe áudio local, ajuste início/duração/ganho, fades lineares, silêncio e ducking durante a voz. A prévia toca as pistas sincronizadas e o exportador aplica fades/ducking e mistura todas usando FFmpeg; receitas de produção podem apontar para WAVs locais relativos ao JSON, sincronizar o início com fala/progresso/palavra e definir os fades em quadros. Fades não nulos são salvos no schema 6. Veja o guia da IA e a [decisão de arquitetura 0013](docs/architecture/0013-multitrack-audio.md).

Receitas v3 aceitam `editing.intensity` de 0 a 100 para escolher entradas e saídas padrão apenas quando a IA não especificou movimento ou duração. Sem esse parâmetro, o padrão antigo é mantido.

Visual Beats podem revelar/esconder elementos, alterar transformações ou animar a câmera. Beats, início de elementos e keyframes aceitam `wordIndex` com timestamp real emitido pelo sintetizador pt-BR local. Os eventos são convertidos em quadros/keyframes do projeto, preservando a edição manual. A timeline exibe os shots e os eventos; alças de corte movem os planos e seus conteúdos relacionados como um único gesto de desfazer. Projetos anteriores continuam abrindo; projeto com vídeo usa schema 5 e fades de áudio persistidos usam schema 6.

O guia lista os campos, limites e restrições da v3. Exemplo completo: [shots e Visual Beats](examples/shots-visual-beats-v3.production.json). A síntese pt-BR grava limites de palavra e pode usá-los em `wordIndex`; confira a [decisão de arquitetura 0012](docs/architecture/0012-word-aligned-beats.md). Veja também a [decisão de arquitetura 0009](docs/architecture/0009-shots-and-visual-beats.md).

## Duração fixa e sincronização proporcional à fala

A receita IA v2 pode controlar a duração final usando `"timingMode":"fit-to-duration"`, `"targetDurationSeconds":90` e `"preserveVoicePitch":true`. O aplicativo sintetiza a voz e depois calcula os quadros a 30 FPS. Aceita alvos opcionais por cena e por fala e combina aceleração limitada a 15% (FFmpeg `atempo`, preservando aproximadamente o tom) com pausas. Quando a narração não cabe, recusa a produção e explica o problema em vez de alterar o texto. Pausas planejadas superiores a 3 segundos após uma fala ou superiores a 5 segundos ao final de uma cena geram avisos de ritmo nos logs e no relatório. O relatório de sucesso registra ainda as janelas de fala e o fator de velocidade calculado.

Objetos, keyframes e câmera podem usar `cueProgress` de 0 a 1 para acompanhar proporcionalmente a duração da fala final. `cueProgress` e `offsetSeconds` são alternativos; não há sincronização por palavra. O projeto tem duração exata em quadros, por exemplo 90 s = 2700 quadros; metadados do MP4/AAC podem refletir atraso de codificação. Consulte o [guia da IA](docs/GUIA-PARA-IA.md) e a ADR 0008.

## Fundação de composição livre (schema v3)

O editor já permite envolver o elemento selecionado em um grupo, gravar keyframes no playhead, escolher curvas linear/ease-in/ease-out/ease-in-out no inspetor e animar centro, zoom e rotação da câmera. Prévia e render usam o mesmo cálculo por frame; projetos v1/v2 continuam abrindo. O projeto de demonstração usa um grupo e movimentos de câmera para servir como prova visual.

Grupos podem ser selecionados pela lista de camadas; arrastar sua faixa move o grupo, seus filhos e os keyframes juntos. Excluir um grupo remove sua árvore. Agora grupos inteiros podem ser salvos e reutilizados como presets, inclusive pelo JSON v2 com `kind: library` e um ID do manifesto pessoal. Curvas de keyframes lineares e suavizadas agora podem ser escolhidas no inspetor para a câmera e os objetos. Conectores e multi seleção continuam pendentes. Receitas v2 e v3 estão disponíveis; consulte as [decisões de arquitetura 0004](docs/architecture/0004-groups-camera-keyframes.md) e [0009](docs/architecture/0009-shots-and-visual-beats.md), além da [direção futura](FUTURO-DO-APP.md).

## Diagnóstico de importação

O botão **Logs** no topo abre os arquivos locais de diagnóstico de importações IA. Cada tentativa gera um arquivo `production-*.jsonl` com data, etapas, mensagens de validação e códigos de término, mesmo quando o JSON é rejeitado antes de iniciar a voz. Os 30 logs mais recentes são mantidos; não são enviados automaticamente, e o conteúdo integral do roteiro/JSON não é registrado. Mensagens de erro podem conter caminhos ou identificadores locais; revise os logs antes de compartilhá-los. Para relatar uma falha, compartilhe o `.jsonl` correspondente e, se puder, o `.production.json` com dados pessoais removidos. Os logs são apenas da importação/produção IA, não de todos os eventos da interface.

## Testar pela interface

1. A demonstração começa com texto e forma animados em uma composição de 12 segundos.
2. Reproduza, navegue pela régua e selecione um objeto na biblioteca ou na prévia.
3. Arraste o objeto na prévia ou ajuste posição, dimensões, texto, cor e tempo no inspetor.
4. Adicione texto, forma, PNG/SVG, vídeo local ou um dos 14 objetos/cenários nativos. Ajuste pose, roupa/pele, movimento, giro e deslocamento. Mova clipes pela timeline; para liberar espaço, reduza a duração pelo inspetor.
5. Use desfazer/refazer, salve com um nome novo e reabra o projeto.
6. Em uma produção gerada, escolha a cena no seletor abaixo da prévia. A biblioteca e a timeline mostram os objetos daquela cena. O áudio permanece fixo enquanto você ajusta visuais.
7. Exporte MP4 com nome novo. Produções geradas incluem narração, trilha e efeitos; projetos visuais sem master de áudio continuam exportando sem som.

Atalhos: `Ctrl+S` salva, `Ctrl+O` abre, `Ctrl+Z` desfaz e `Ctrl+Shift+Z` refaz. Campos numéricos e de cor aplicam a alteração ao perder o foco; o conteúdo do texto é atualizado durante a digitação. Navegar pela régua pausa a reprodução. A recuperação automática fica disponível pelo botão na biblioteca após uma edição.

## Estrutura para desenvolvimento

```text
apps/
  desktop/          aplicação Qt, ponte C++ e painéis QML
  render-worker/    processo de renderização e codificação
  production-worker/ validação, voz SAPI e montagem automática
modules/
  core/             documento e edição, C++ sem Qt
  storage/          JSON, lock, revisões e escrita atômica
  composition/      composição compartilhada entre prévia e exportação
  media/            PCM/WAV e trilha procedural
  production/       receita, catálogo, guia e compilador audiovisual
examples/           roteiro sintético e receita reproduzível
scripts/            preparo, ambiente, build, execução e smoke audiovisual
tests/              comandos, tempo, persistência e composição
docs/architecture/  decisões e limites do protótipo
.tools/             SDK e compilador locais; fora do Git
build/              binários e arquivos de compilação; fora do Git
artifacts/          capturas, MP4s e relatórios de teste; fora do Git
```

Comandos PowerShell para desenvolvimento:

```powershell
.\scripts\setup.ps1
.\scripts\build.ps1 -Test
.\scripts\smoke.ps1
.\scripts\smoke-production.ps1
.\scripts\smoke-workspace.ps1
.\scripts\update-ai-guide.ps1
.\scripts\run.ps1
```

Leia a [arquitetura da produção por IA](docs/architecture/0002-ai-directed-production.md) e a [fundação nativa](docs/architecture/0001-native-prototype.md). O [plano futuro](FUTURO-DO-APP.md) mantém o estado atual e a visão de produto.

A persistência da coleção e os limites de exclusão estão na [arquitetura de projetos e biblioteca](docs/architecture/0003-projects-and-library.md).

A composição por grupos/câmera e a direção v2 estão em [0004](docs/architecture/0004-groups-camera-keyframes.md) e [0005](docs/architecture/0005-production-v2-personal-library.md). Os presets de grupos são detalhados na [ADR 0006](docs/architecture/0006-reusable-group-presets.md). Shots e Visual Beats estão em [0009](docs/architecture/0009-shots-and-visual-beats.md). A intensidade da edição e seus defaults estão em [0010](docs/architecture/0010-editing-intensity.md).

A evolução proposta a partir do vídeo de referência está em [Futuro do aplicativo](FUTURO-DO-APP.md): composição livre, grupos, câmera, documentos, vídeo e direção por IA. Esse documento é planejamento, não uma lista de recursos já implementados.

## Estado deste corte

Há biblioteca vetorial autoral, quatro poses de personagem, seis movimentos de entrada, PNG/SVG/vídeo local, rotação/escala/deslocamento, receitas v1/v2/v3, shots e transições editáveis, Visual Beats, narração pt-BR local, sincronização por palavra, trilha/efeitos procedurais, até 32 pistas com ducking/fades, forma de onda e reprodução audiovisual. Continuam pendentes destaques de palavra em legenda, áudio sidechain na prévia, automação de volume por pontos e ferramentas avançadas de timeline. Projetos v1–v6 continuam abrindo, e as receitas v3 continuam compatíveis.

A IA externa prepara a receita; o app executa capacidades reais. Personagens são objetos parametrizados, sem edição individual dos ossos ou sincronização labial. SVG não é convertido automaticamente em partes nativas. Também faltam automação de ritmo a partir do conteúdo, curvas Bézier personalizadas/trajetórias e outros formatos de tela.

Alterar texto falado ou os volumes da trilha exige recompilar a receita. Editar uma legenda visual não regenera a voz. Imagens e áudio dependem dos caminhos locais; o projeto ainda não é um pacote portátil. Use uma janela do editor por vez. Instalador e redistribuição pública das dependências ficam para uma etapa própria.
