# ADR 0035 — Diagnóstico visual e ajuste de texto em premium

Data: 27/10/2026. Estado: implementado em `teste-biblioteca`.

## Contexto

Receitas corretas no schema ainda podiam produzir títulos cortados, slots premium que não comportavam texto personalizado ou alvos de câmera enquadrados fora da área útil. A revisão artística continua humana, mas o compositor já resolve os bounds reais, as transformações e os estados por frame necessários para detectar alguns problemas objetivos antes da exportação.

## Decisões

- `VisualQualityAnalyzer` fica em `modules/composition` e consulta `RenderSnapshot`; não cria um segundo motor de avaliação. Seus diagnósticos são consultivos, não editam o projeto e não bloqueiam a exportação.
- A análise verifica texto premium/essencial fora do canvas, invasão da margem segura em papéis relevantes, colisão texto-texto ou texto-legenda, contraste quando existe uma forma sólida identificável atrás, estados imediatamente antes/no/depois de `show`, e bounds dos alvos de `camera.path` no ponto resolvido.
- A análise amostra início, fim e fim do movimento de textos, limites de shots, pontos e quartos de câmera, e eventos de beat. `sampledFrames` no relatório torna a cobertura observável. A checagem não é varredura de todos os quadros e não certifica conteúdo visual ou intenção estética. Sobreposições que possam ser intencionais devem ser julgadas por uma pessoa.
- O relatório tem formato JSON `studio-dark-visual-quality` versão 1, inclui severidade, cena, shot, elemento, intervalo de frames, descrição, evidência e sugestão. O editor abre a análise antes do seletor de exportação; a mesma análise pode ser executada pelo render worker sem exportar vídeo. Um `show` válido no tempo, mas com bounds transformados totalmente fora do viewport, recebe aviso; revele parcialmente fora da tela apenas quando isso fizer parte da direção.
- Manifestos premium podem declarar `textLayout` por slot: `mode: "shrink"`, `minimumFontSize` e `padding`. Qt mede e quebra as linhas com a fonte atual; o compositor usa busca binária determinística para encontrar o maior tamanho que cabe sem cruzar o mínimo. O conteúdo nunca é truncado. Quando há redução, o relatório inclui `text-adapted` com severidade informativa no primeiro quadro visível; se nem o mínimo funcionar, o texto permanece íntegro e a análise alerta.
- A serialização desses dados usa schema 15. Projetos anteriores sem `textLayout` continuam com o comportamento legado; inserir um premium com políticas atualiza a versão do projeto.
- Para slots de imagem premium por receita, o contrato definitivo é um caminho relativo seguro à pasta da receita. Caminhos absolutos, UNC, URL, traversal e symlinks resolvidos fora da pasta são recusados pelo parser. PNG/JPEG/WebP/BMP são verificados quanto a existência, leitura, limites existentes de arquivo/dimensões e decodificação. O worker copia a mídia validada ao workspace. Isso não é empacotamento portátil entre máquinas.

## Consequências

O ajuste de texto acontece no QPainter usado por preview e exportação, sem alterar nós legados fora da política declarada. O relatório ajuda a encontrar defeitos concretos, mas não adivinha papéis semânticos desconhecidos, leitura sobre fotografias, fatos, gosto visual, intenção de crop ou todas as sobreposições desejadas. A janela não deve tratar “nenhum aviso” como aprovação de publicação. Uma produção complexa pode ter muitos frames relevantes; a análise é mais leve que exportar, mas não tem um custo constante.

O teste de regressão cobre texto que reduz, texto indivisível que continua em overflow, texto fora do canvas, colisão com legenda, alvo de câmera inseguro e uma cena simples sem falsos avisos. Testes de premium validam os nove layouts nos três canvases oficiais.
