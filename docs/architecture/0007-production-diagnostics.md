# ADR 0007 — diagnósticos locais de receitas IA

Data: 24/10/2026. Estado: código entregue na master; validação Windows ainda pendente.

## Problema

Erros de importação e da síntese local eram exibidos apenas na barra de status e se perdiam quando o usuário saía do aplicativo. Isso dificultava reproduzir falhas de JSON produzido por uma IA externa.

## Decisão

- Cada tentativa de `Importar produção IA` abre seu próprio arquivo `production-<timestamp>-<id>.jsonl`, sob `QStandardPaths::AppLocalDataLocation/logs` (ou `<recoveryDirectory>/logs` nos testes).
- O arquivo registra um JSON compacto por linha com `format`, `timestamp`, `event`, `detail` e `progress` quando disponível.
- Incluir erros de acesso, sintaxe (linha/coluna), validação prévia, eventos do worker, pedido de cancelamento, erro do processo e código/estado de saída.
- Expor o botão **Logs** no cabeçalho para abrir a pasta no explorador; preservar os 30 arquivos mais recentes, sem exportação ou upload automático.
- Na tela inicial, manter uma ação explícita **Importar receita IA** e fazer os seletores de projeto/receita usarem um filtro `.json` simples, com alternativa para todos os arquivos. O importador identifica uma receita pelo campo `format`, não pela extensão composta do arquivo.
- Nunca copiar a receita bruta, o roteiro nem WAV para o log. Uma mensagem devolvida pelo worker pode conter nomes, caminhos ou informação pessoal incidental; orientar o usuário a revisar antes de compartilhar.
- O diagnóstico não substitui o relatório `production-report.json` de uma produção bem-sucedida, e neste corte não cobre todos os eventos de editor/render.

## Verificação

Um teste do controlador importa JSON propositalmente inválido e verifica que o log registra o começo e o erro sem incluir bytes da receita. Um teste de produção confirma informação de localização de erro de sintaxe. O comportamento do processo SAPI ainda precisa de smoke real no Windows.
