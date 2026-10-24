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
2. Envie à IA o roteiro e o [guia do aplicativo](docs/GUIA-PARA-IA.md). O botão **Guia IA** também salva uma cópia atualizada.
3. Peça um JSON de produção seguindo o guia e salve a resposta como `.production.json`.
4. No editor, use **Importar produção IA**. O app valida o plano, gera a voz local, mede os tempos e monta as cenas.
5. Reproduza, ajuste os objetos e exporte MP4.

Exemplo pronto: [roteiro sobre carros](examples/ROTEIRO-CARROS-CAROS.md) e [direção correspondente](examples/carros-caros.production.json). O botão **Criar vídeo: carros caros** monta esse exemplo diretamente no editor; para criar o MP4, use Exportar.

## Testar pela interface

1. A demonstração começa com texto e forma animados em uma composição de 12 segundos.
2. Reproduza, navegue pela régua e selecione um objeto na biblioteca ou na prévia.
3. Arraste o objeto na prévia ou ajuste posição, dimensões, texto, cor e tempo no inspetor.
4. Adicione texto, forma, PNG/SVG ou um dos 14 objetos/cenários nativos. Ajuste pose, roupa/pele, movimento, giro e deslocamento. Mova clipes pela timeline; para liberar espaço, reduza a duração pelo inspetor.
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

Leia a [arquitetura da produção por IA](docs/architecture/0002-ai-directed-production.md) e a [fundação nativa](docs/architecture/0001-native-prototype.md). O [plano inicial](PLANO-INICIAL.md) mantém a visão de produto.

A persistência da coleção e os limites de exclusão estão na [arquitetura de projetos e biblioteca](docs/architecture/0003-projects-and-library.md).

A evolução proposta a partir do vídeo de referência está em [Futuro do aplicativo](FUTURO-DO-APP.md): composição livre, grupos, câmera, documentos, vídeo e direção por IA. Esse documento é planejamento, não uma lista de recursos já implementados.

## Estado deste corte

Há biblioteca vetorial autoral, quatro poses de personagem, seis movimentos de entrada, PNG/SVG, rotação/escala/deslocamento, produção declarativa, narração pt-BR local, legendas por trecho medido, trilha/efeitos procedurais, forma de onda e reprodução audiovisual. Permanecem histórico, salvamento por revisão, cópia anterior, recuperação e render em processo separado. Projetos v1 são migrados em memória para v2.

Ainda não há clipes de vídeo, importação livre de áudio na interface, mixagem em pistas independentes, câmera global, keyframes arbitrários, gráficos estatísticos ou chat de IA interno. A IA externa prepara a receita; o app executa capacidades reais. Personagens são objetos parametrizados, sem edição individual dos ossos ou sincronização labial. SVG não é convertido automaticamente em partes nativas.

Alterar texto falado ou os volumes da trilha exige recompilar a receita. Editar uma legenda visual não regenera a voz. Imagens e áudio dependem dos caminhos locais; o projeto ainda não é um pacote portátil. Use uma janela do editor por vez. Instalador e redistribuição pública das dependências ficam para uma etapa própria.
