# Verificação — projetos e biblioteca completa

Data: 24/10/2026. Windows x64, Qt 6.8.3, MinGW 13.1.0, Release.

## Testes aprovados

- `scripts/build.ps1 -Test`: quatro suítes CTest aprovadas, incluindo `workspace_projects_library`.
- A suíte nova executou 55 verificações: criação e salvamento inicial, importação/registro, duplicação, renomeação, exclusão/restauração, navegação, busca, categorias, nativos, variações, cópia de mídia, persistência e concorrência.
- Exclusão de categoria preservou seus itens e os moveu para Sem categoria. A categoria padrão permaneceu protegida.
- Remoção de nativo não alterou o registro de renderização nem invalidou o projeto aberto.
- Uma imagem importada continuou disponível após remover o arquivo original de teste. Removê-la da biblioteca também preservou o arquivo usado pelo projeto.
- Variações de imagem preservaram opacidade e arquivo próprio. Variações nativas preservaram estilo e receberam IDs novos a cada inserção.
- Exclusão do projeto aberto reteve o arquivo e a mídia; restauração reabriu todo o conteúdo. Renomear pela tela inicial preservou a navegação.
- Escrita com revisão obsoleta foi recusada. Atualizar resolveu a concorrência. Um catálogo corrompido não foi substituído silenciosamente e a cópia anterior permaneceu disponível.
- `scripts/smoke-workspace.ps1`: capturas da tela vazia, tela com dois projetos e miniaturas, biblioteca completa e retorno ao editor, com perfil de dados isolado. Sem erros QML detectados nos logs.
- `scripts/smoke.ps1`: prévia/render, cancelamento, preservação de arquivo existente e mídia ausente continuaram aprovados.
- `scripts/smoke-production.ps1`: geração real de voz, projeto com 43 elementos, reprodução audiovisual e MP4 com 1.517 frames aprovados. Comparação de seis frames manteve PSNR médio próximo de 40,67 dB.

## Evidências locais

`artifacts/workspace-smoke-20261024-171259/` contém imagens, logs, perfil isolado e relatório das telas. Os smokes visual e audiovisual da mesma data ficam em diretórios próprios dentro de `artifacts/`. Nenhum desses perfis contém ou modifica dados reais do usuário.

As capturas foram inspecionadas visualmente. Não foi executada uma sequência de cliques pela ferramenta de controle do computador; os casos de uso foram verificados pela API C++ e a interface pelo modo de captura do aplicativo. Exclusão permanente e limpeza de disco não fazem parte desta entrega.
