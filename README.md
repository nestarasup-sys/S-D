# Studio Dark · Native Lab

Protótipo novo em C++20 e Qt Quick/QML para testar a estrutura do futuro editor audiovisual.

## Abrir

Clique em **ABRIR-STUDIO.bat**. As ferramentas locais já preparadas ficam em `.tools/`; o executável compilado fica em `build/bin/studio_dark.exe`. O BAT configura os caminhos necessários e abre a janela sem terminal permanente. Não execute apenas o EXE fora desse ambiente de desenvolvimento.

Em outra máquina Windows x64 com Python 3.12 e internet, use **PREPARAR-E-COMPILAR.bat**. Downloads não exigem instalação global de Qt ou C++ e ficam na pasta do projeto.

## Testar pela interface

1. A demonstração começa com texto e forma animados em uma composição de 12 segundos.
2. Reproduza, navegue pela régua e selecione um objeto na biblioteca ou na prévia.
3. Arraste o objeto na prévia ou ajuste posição, dimensões, texto, cor e tempo no inspetor.
4. Adicione texto, forma ou uma imagem local. Mova clipes pela timeline; para liberar espaço, reduza a duração pelo inspetor.
5. Use desfazer/refazer, salve com um nome novo e reabra o projeto.
6. Exporte um MP4 com nome novo. Este corte exporta composição visual **sem áudio**.

Atalhos: `Ctrl+S` salva, `Ctrl+O` abre, `Ctrl+Z` desfaz e `Ctrl+Shift+Z` refaz. Campos numéricos e de cor aplicam a alteração ao perder o foco; o conteúdo do texto é atualizado durante a digitação. Navegar pela régua pausa a reprodução. A recuperação automática fica disponível pelo botão na biblioteca após uma edição.

## Estrutura para desenvolvimento

```text
apps/
  desktop/          aplicação Qt, ponte C++ e painéis QML
  render-worker/    processo de renderização e codificação
modules/
  core/             documento e edição, C++ sem Qt
  storage/          JSON, lock, revisões e escrita atômica
  composition/      composição compartilhada entre prévia e exportação
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
.\scripts\run.ps1
```

Leia a [decisão de arquitetura](docs/architecture/0001-native-prototype.md) para dependências, protocolo de persistência e limites. O [plano inicial](PLANO-INICIAL.md) mantém a visão de produto.

## Estado deste corte

Há composição de texto/formas/imagens, animação de entrada, reprodução, seleção e arraste no canvas, timeline de objetos, inspetor, histórico, gravação com revisão, cópia anterior, recuperação automática e renderização MP4 em processo separado.

Ainda não há importação de vídeo/áudio, waveform, keyframes editáveis, timeline profissional completa, IA integrada, projetos portáteis ou instalador. Imagens dependem dos arquivos originais. Use uma janela do editor por vez neste protótipo. As ferramentas baixadas são dependências de desenvolvimento; este repositório ainda não é um pacote de distribuição pública.
