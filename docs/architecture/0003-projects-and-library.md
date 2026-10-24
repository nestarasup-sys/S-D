# ADR 0003 — tela inicial, coleção de projetos e biblioteca gerenciável

Data: 24/10/2026. Implementação: protótipo 0.3.

## Organização da interface

A navegação principal separa Projetos, Biblioteca completa e Editor. Trocar entre telas pausa a reprodução e preserva o documento em memória. Trocar de documento continua exigindo a decisão sobre alterações não salvas. Argumentos `--project` abrem diretamente o editor; a inicialização normal mostra Projetos.

`HomePage.qml` apresenta coleção e lixeira de projetos. `AssetLibraryPage.qml` apresenta busca, categorias, importação, variações e itens excluídos. `WorkspaceController` adapta os casos de uso para QML e coordena o documento aberto. Operações persistentes ficam em `WorkspaceStore`, dentro do módulo de armazenamento; o núcleo permanece sem Qt.

## Dados e persistência

Os dados ficam sob `QStandardPaths::AppLocalDataLocation/workspace`, fora do repositório:

```text
workspace/
  catalogue.json             # Registro de projetos, categorias e itens
  catalogue.json.previous    # Última versão íntegra
  projects/<id>/project.studio.json
  library/media/<sha256>.<ext>
  cache/<hash-id>.png         # Prévias regeneráveis
```

O catálogo tem formato `studio-dark-workspace`, versão 1. Cada mutação usa lock cooperativo, comparação SHA-256 da revisão, cópia anterior e gravação atômica. Uma janela com revisão obsoleta recebe orientação para Atualizar; catálogos inválidos não são substituídos silenciosamente.

Um novo projeto recebe identidade independente e arquivo próprio. Importações registram o caminho do arquivo existente. Abrir ou salvar pelo editor atualiza a coleção; projetos legados da pasta de produções local são descobertos quando a coleção ainda não tem projetos. Cópias reutilizam referências de mídia e recebem outro ID de documento, sem alterar o original.

## Biblioteca e categorias

A biblioteca inicial deriva dos 14 IDs do registro nativo. Metadados de nome, categoria e exclusão pertencem ao catálogo do usuário, não ao motor. Isso permite reorganizar sem modificar código, receitas ou projetos salvos.

Arquivos importados são copiados e nomeados pelo hash do conteúdo. Os limites atuais são 32 MiB por mídia, 4 MiB para SVG e 40 megapixels na validação de imagens. Variações armazenam um documento mínimo com um nó validado; no uso, recebem novo ID, cena e intervalo. Pose, cores, tamanho, opacidade e comportamento permanecem reutilizáveis. Imagens/SVG em variações têm cópia própria, independente da origem.

Categorias têm IDs estáveis e nomes editáveis. Exclusão de categoria move itens, inclusive os excluídos, para o ID fixo `uncategorized`, evitando referências quebradas. A categoria de destino padrão não pode ser excluída.

## Semântica de exclusão

Projetos e itens são excluídos da coleção por tombstone, sem apagar ou mover os arquivos físicos. A interface oferece restauração. Isso também preserva mídia compartilhada por cópias e por elementos previamente inseridos em outros projetos. Remover um nativo não remove seu código, seu ID nem sua capacidade de renderização.

Ao excluir o projeto atualmente aberto, alterações pendentes são salvas com o protocolo de revisão antes de fechar o documento; se o salvamento falhar, a exclusão é interrompida. A exclusão exige confirmação na interface. Não há purga de disco ou coleta de mídia sem referências nesta etapa.

## Prévias e integridade

Prévia de projeto e variação usam `CompositionPainter`. Thumbnails nativos são fornecidos por um image provider local, sem rede. Falhas de miniatura não invalidam o documento; o cache não é a fonte de verdade.

Nome do projeto no catálogo é uma versão de uma linha, limitada a 120 caracteres, sem reescrever o documento durante importação. Os cartões usam texto simples, não HTML. Os arquivos do usuário não são enviados a terceiros.

## Testes e limites

`workspace_projects_library` exercita criação, cópia, renomeação, exclusão/restauração, categorias, arquivos próprios, variações, persistência, concorrência e catálogo corrompido. `scripts/smoke-workspace.ps1` abre e captura as telas vazia, preenchida, biblioteca e editor em perfil isolado. Os smokes visual e audiovisual existentes continuam obrigatórios para os contratos de produção.

Ainda não há categorias de projetos, sincronização, purga permanente, importação em lote ou portabilidade de todos os assets de um projeto. O guia estático continua descrevendo capacidades do motor; a organização e as variações privadas da biblioteca não são publicadas automaticamente no guia. A recomendação de usar uma janela por vez continua válida para a cópia de recuperação do editor.
