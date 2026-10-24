# Studio Dark — plano inicial do novo editor

Data: 24/10/2026. Estado: proposta de produto e arquitetura; implementação ainda não iniciada.

## 1. Direção do projeto

Criar um estúdio audiovisual para Windows em que seja possível transformar uma ideia em vídeo, dirigir cada detalhe e continuar editando tudo depois. O foco inicial será documentários e vídeos explicativos, com espaço para motion design, conteúdo de canal e outros formatos.

O diferencial precisa ser liberdade com organização: construir cenas autorais, combinar mídia e animação, trabalhar manualmente ou com IA e reaproveitar identidades, personagens e composições. Uma biblioteca de modelos ajuda a começar; o editor precisa permitir ir além dela.

Este será um projeto novo, com modelo de dados, interface e evolução próprios. O D-E-V1 serve como referência das necessidades e aprendizados. Não haverá clonagem do repositório nem obrigação inicial de compatibilidade com seus arquivos. Uma importação futura poderá converter uma cópia, acompanhada de relatório de perdas.

**Hipóteses iniciais:** Windows primeiro, projetos e mídia locais, interface em português, uso por uma pessoa e IA externa como primeira integração. Essas hipóteses podem mudar sem exigir a reconstrução do núcleo.

## 2. O que foi observado na referência

Consulta ao repositório em 24/10/2026. Análise documental e de trechos de código; o aplicativo não foi executado. Recursos descritos no README não equivalem a qualidade ou desempenho comprovados nesta análise.

| Evidência | Aprendizado para o novo projeto |
| --- | --- |
| README v0.18 descreve produção por IA, voz, legendas, camadas, evidências, pacotes e exportação local | A necessidade já é de um estúdio de produção, além de uma timeline básica. Organizar capacidades por módulos e fluxo de trabalho. |
| O início de `src/Editor.tsx` reúne estado e chamadas relacionadas a reprodução, voz, exportação, OCR e B-roll | Separar interface, casos de uso e integrações desde a primeira entrega. Esta é uma observação do trecho lido, sem diagnóstico completo do arquivo. |
| `src/model/project.ts` fixa 30 FPS, 1920×1080 e valida mídia contra uma URL de serviço específica | Representar tempo, formato e referências de mídia independentemente da porta do serviço e da resolução. |
| A arquitetura documentada compartilha composição entre prévia e exportação | Preservar esse princípio no projeto novo e provar sua consistência com testes visuais. |
| A documentação registra revisões, snapshots e proteção contra mudanças concorrentes | Recuperação e controle de revisão pertencem à fundação do produto. |

Fontes consultadas:

- [README da referência](https://github.com/nestarasup-sys/D-E-V1/blob/main/README.md).
- [Dependências e comandos](https://github.com/nestarasup-sys/D-E-V1/blob/main/package.json).
- [Modelo de projeto](https://github.com/nestarasup-sys/D-E-V1/blob/main/src/model/project.ts).
- [Interface do editor](https://github.com/nestarasup-sys/D-E-V1/blob/main/src/Editor.tsx).
- [Decisão histórica de arquitetura](https://github.com/nestarasup-sys/D-E-V1/blob/main/docs/VIDEO_ARCHITECTURE.md). Esse documento descreve uma etapa anterior; não representa sozinho o estado v0.18.

## 3. O que significa ser melhor

1. **Controle real:** selecionar um objeto na imagem, editar suas propriedades e animá-lo sem alterar o restante do episódio.
2. **Montagem confortável:** mover, cortar, aparar, encaixar e agrupar clipes, com zoom, seleção múltipla, atalhos e desfazer/refazer.
3. **Confiança:** salvar, fechar, recuperar e exportar sem perder trabalho; mostrar problemas e como resolvê-los.
4. **Criatividade extensível:** construir composições novas com objetos, efeitos e comportamentos combináveis.
5. **Automação útil:** a IA utiliza as mesmas operações da edição manual, com resultado visível e possibilidade de desfazer.
6. **Desempenho verificável:** manter interação utilizável enquanto tarefas pesadas são executadas fora da interface.

O número de presets não será a principal medida de evolução. Cada recurso precisa funcionar no ciclo completo: criar, editar, salvar, reabrir e exportar.

## 4. Experiência desejada

Fluxo principal: **ideia → roteiro → storyboard → montagem → animação → som e legendas → revisão → exportação**. Será possível começar diretamente pela montagem e voltar a qualquer etapa.

Na área de edição, a biblioteca fica à esquerda, a prévia no centro, o inspetor do objeto selecionado à direita e a timeline abaixo. Painéis poderão ser redimensionados; layouts salvos ficam para depois da primeira entrega. A interface deve apresentar nomes de ações e mensagens compreensíveis, sem exigir que o usuário manipule JSON ou terminal.

Áreas previstas:

| Área | Responsabilidade |
| --- | --- |
| Projetos | Criar, abrir, duplicar, recuperar e localizar projetos. |
| Roteiro e storyboard | Organizar blocos narrativos e relacioná-los às sequências. |
| Montagem | Editar clipes, trilhas e sincronização. |
| Composição | Editar objetos, grupos, texto, câmera e animação. |
| Som e legendas | Voz, música, efeitos, mixagem e legendas. |
| Revisão e exportação | Conferir problemas, comparar mudanças e acompanhar arquivos finais. |

Essas áreas serão entregues progressivamente; não são seis telas obrigatórias na primeira versão.

## 5. Fundação técnica proposta

Usar um **monólito modular**: um produto e um repositório, com limites claros entre módulos. Processos separados serão reservados para renderização e tarefas pesadas. Não iniciar com microserviços.

```text
Interface desktop e integração externa
                  ↓
Casos de uso e comandos de edição
                  ↓
Núcleo: projeto, tempo, composição e validação
                  ↓
Contratos para armazenamento, mídia, render e IA
                  ↓
Adaptadores locais e provedores externos
```

Regras de dependência:

- O núcleo não conhece React, portas HTTP, caminhos específicos do Windows nem fornecedor de IA.
- Componentes de tela não escrevem arquivos nem implementam regras de edição.
- Interface e automação chamam os mesmos comandos validados.
- Prévia e exportação recebem a mesma composição normalizada, com mesmas fontes e regras de tempo.
- Render, análise de mídia, geração de voz e transcrição têm progresso, cancelamento e relatório de falha.
- Projetos e mídias do usuário ficam fora do repositório de código.

### Tecnologias candidatas e decisões pendentes

TypeScript para contratos e lógica compartilhada; React para a interface; Tauri como candidato a aplicativo desktop. Remotion é candidato a renderização de composições e FFmpeg a processamento de mídia. Essas escolhas devem ser confirmadas por um protótipo, verificação de distribuição/licenças e medição no computador de destino. Não fixar versões nem instalar dependências nesta etapa.

O protótipo deve comparar prévia em tempo real, exportação, codecs, memória, instalação e custo de manutenção. Trocar um adaptador deve ser possível; trocar um motor ainda pode exigir trabalho significativo, portanto essa validação precede a expansão dos efeitos.

### Estrutura prevista

```text
apps/
  desktop/                 # Janela e composição da interface
  worker/                  # Tarefas pesadas e renderização
packages/
  core/                    # Modelo, tempo, validação e comandos
  application/             # Casos de uso e contratos de integração
  timeline/                # Interação e apresentação da montagem
  composition/             # Avaliação visual e animação
  media/                   # Importação, metadados, proxies e cache
  storage/                 # Persistência, revisões e recuperação
  automation/              # Comandos externos e propostas da IA
  ui/                      # Componentes e padrões visuais
docs/
  decisions/               # Decisões e seus motivos
  product/                 # Fluxos, escopo e critérios de aceite
tests/
  fixtures/                # Projetos sintéticos e mídia de teste
```

Criar pastas e pacotes quando houver implementação que justifique cada um; a árvore é uma orientação de limites.

## 6. Modelo de projeto que permite crescer

| Entidade | Função |
| --- | --- |
| Projeto | Identidade, versão do formato, revisão e configurações. |
| Sequência | Montagem com resolução, taxa de quadros e duração próprias. |
| Trilha e clipe | Posição na sequência, trecho de origem e vínculo opcional com outros clipes. |
| Composição e nó | Árvore de objetos: texto, imagem, vídeo, forma, grupo e câmera. |
| Propriedade animada | Valor inicial e keyframes com curvas de interpolação. |
| Asset | Referência lógica à mídia, hash, metadados, origem e direitos cadastrados. |
| Legenda | Texto e intervalo; palavras alinhadas apenas quando houver informação confiável. |
| Bloco de roteiro | Texto narrativo relacionado a cenas, voz e evidências. |
| Evidência | Alegação, origem e situação da revisão editorial. |
| Job e proposta | Tarefa acompanhável ou conjunto de alterações sobre uma revisão do projeto. |

Decisões de base:

- IDs estáveis ligam entidades. Mídia usa `assetId`, resolvido pelo armazenamento, em vez de URL fixa no documento.
- Tempo tem representação racional e conversões explícitas entre mídia e sequência. O primeiro piloto pode usar 30 FPS, mas o formato deve comportar 24, 25, 30 e taxas fracionárias sem acumular arredondamentos.
- Arquivo de projeto tem versão de schema e migrações explícitas. Abrir uma versão futura incompatível mostra orientação e preserva o original.
- Roteiro organiza a narrativa; a montagem define o tempo audiovisual. Alterar um título ou bloco não deve deslocar áudio implicitamente.
- Estado visual, seleção, painéis e caches ficam separados do documento exportável.

## 7. Edição manual e IA pelo mesmo caminho

Operações como `AddClip`, `MoveClip`, `TrimClip`, `SetProperty`, `AddKeyframe` e `ReplaceNarration` passam por validação e são aplicadas como transações. Um comando composto produz uma etapa coerente no histórico de desfazer.

Exemplo: pedir “troque o fundo desta cena e aproxime o gráfico” deve alterar os objetos selecionados, preservar voz e legendas e registrar uma operação reversível. Pedir para regenerar a voz deve mostrar quais tempos precisam ser recalculados e quais ajustes manuais serão preservados.

Integração inicial por API/CLI documentada, utilizável pelo assistente externo. Fluxo padrão de IA: ler capacidades e revisão → preparar comandos → validar → apresentar comparação → aplicar conforme a ação solicitada → registrar resultado. A edição manual não exige confirmações repetitivas. Conflitos de revisão são explicados sem sobrescrever silenciosamente o trabalho mais recente.

Fornecedores de texto, voz, transcrição, busca e geração visual entram por adaptadores com capacidades declaradas. Chat interno é uma possibilidade futura, não uma dependência para editar ou exportar. Chamadas pagas devem informar estimativa e respeitar limites configurados pelo usuário.

## 8. Escopo por prioridade

| Prioridade | Conteúdo |
| --- | --- |
| P0 — primeiro editor utilizável | Projetos locais, mídia, prévia, timeline básica, seleção no canvas, inspetor, texto/imagem/vídeo/áudio, autosave, recuperação, desfazer/refazer e MP4. |
| P1 — liberdade criativa | Grupos, keyframes, curvas, máscaras básicas, transições, waveform, proxies, legendas, kit visual e composições reutilizáveis. |
| P2 — produção assistida | Roteiro/storyboard, voz e transcrição por adaptadores, propostas da IA, vínculo com fontes, B-roll com procedência e automação em lote. |
| P3 — expansão | Rigs de personagens, animação avançada, curvas por editor gráfico, extensões, sequências aninhadas, novos formatos e fluxos de equipe. |

Modelo de automação e separação dos módulos começam em P0; integrações completas de IA chegam em P2. Sequências aninhadas devem ser antecipadas no desenho, sem implementação prematura.

Não entram na primeira entrega: 3D completo, colaboração simultânea, marketplace, sincronização em nuvem e dezenas de integrações de IA.

## 9. Etapas e critérios de passagem

### Etapa 0 — Definir a primeira experiência

Entregar um briefing de vídeo de 30–60 segundos, mapa das telas, esboço visual e decisões sobre armazenamento e renderização. Usar mídia sintética ou autorizada. Levantar o hardware real; a máquina citada na referência é apenas uma hipótese até confirmação.

**Concluída quando:** o fluxo mostra o que o usuário fará do projeto vazio ao MP4, o P0 está delimitado e o protótipo técnico resolve as principais dúvidas do motor.

### Etapa 1 — Provar o ciclo completo

Construir uma sequência curta com imagem, vídeo, texto e áudio. Importar, posicionar, visualizar, salvar, reabrir e exportar. Preparar arquivo versionado, escrita atômica e cópia de recuperação.

**Concluída quando:** o vídeo exportado reproduz os cortes e objetos da prévia; reabrir preserva o estado; uma falha simulada de salvamento mantém ao menos a última versão íntegra.

### Etapa 2 — Tornar a edição confortável

Adicionar timeline com arraste, corte, ajuste de bordas, snap, seleção múltipla, zoom, bloqueio de trilha e ligação explícita entre áudio e vídeo. Integrar seleção no canvas, inspetor e histórico transacional.

**Concluída quando:** mover um visual preserva narração independente; desfazer/refazer restaura o documento; uma pessoa consegue montar o piloto sem editar arquivos nem usar terminal.

### Etapa 3 — Abrir a criação visual

Adicionar objetos e grupos, keyframes, curvas básicas, câmera 2D, templates editáveis e identidade de canal. Produzir três composições diferentes usando os mesmos recursos fundamentais: uma explicação, um gráfico e uma cena documental.

**Concluída quando:** objetos continuam editáveis após salvar e reabrir; propriedades animadas têm resultados consistentes na prévia e no arquivo final.

### Etapa 4 — Integrar produção assistida

Expor comandos externos, comparação de propostas e adaptadores iniciais de voz e transcrição. Conectar roteiro, storyboard e legendas. Regenerar apenas o bloco solicitado, preservando ajustes que não dependem dele.

**Concluída quando:** uma alteração por IA percorre a mesma validação da edição manual; revisão antiga é recusada; falta de fornecedor ou falha de voz é visível e não destrói a montagem existente.

### Etapa 5 — Validar um episódio e distribuir

Testar episódio de 5–10 minutos, proxies/cache, reconexão de mídia ausente, fila de exportação, instalação e atualização. Medir interação e memória e corrigir gargalos antes de ampliar o catálogo criativo.

**Concluída quando:** o episódio abre, edita e exporta na máquina de destino; cancelamento e falha de render preservam o projeto; instalação e atualização mantêm dados do usuário.

Não fixar datas antes da Etapa 0. Cada etapa poderá ser dividida em entregas menores, sempre deixando uma versão demonstrável e um commit local.

## 10. Qualidade e desempenho

- Testes do núcleo: tempo, cortes, limites de clipes, vínculos, transações, desfazer/refazer e migrações.
- Integração: salvar/reabrir, recuperação após interrupção, revisão concorrente, mídia ausente e jobs cancelados.
- Visuais: frames representativos, limites de cortes, fontes e composição em prévia/exportação; tolerância definida conforme o motor e o codec.
- Audiovisuais: conferir início/fim, duração e sincronismo ouvindo o MP4 real. Testes automáticos não substituem essa revisão.
- Produto: criar e exportar o piloto pela interface, incluindo mensagens de erro e recuperação.

Metas iniciais de investigação: resposta às interações comuns abaixo de 100 ms e prévia de 30 FPS no piloto com proxies quando necessário. Medir tempos de abertura, busca na timeline, RAM e exportação em projetos de 1, 5 e 10 minutos. Essas são metas propostas, não resultados já alcançados. Registrar equipamento, mídia e configurações em cada medição.

O serviço local, se necessário, terá autenticação de sessão, acesso restrito e validação dos arquivos recebidos. Templates iniciais serão dados declarativos validados; execução de código de extensões exigirá um desenho separado.

## 11. Riscos e respostas

| Risco | Resposta planejada |
| --- | --- |
| Crescer antes de ter uma montagem utilizável | Validar o ciclo completo e a edição manual antes de multiplicar integrações. |
| Prévia diferente do exportado | Mesma composição, fontes e avaliação temporal; comparação de amostras. |
| Interface lenta com mídias grandes | Workers, proxies, caches descartáveis e timeline virtualizada conforme medição. |
| Perder edição ao regenerar voz ou aplicar IA | Revisões, transações, dependências explícitas e histórico recuperável. |
| Ficar preso a um motor ou fornecedor | Contratos documentados e protótipo antes de expandir recursos específicos. |
| Formato de projeto ficar limitado | Separar narrativa, sequência, composição e mídia; testar migrações desde o início. |
| Confundir recurso descrito com recurso pronto | Marcar cada entrega como planejada, implementada, testada ou validada no Windows. |

## 12. Próxima ação recomendada

Executar a Etapa 0 e construir uma primeira fatia funcional: **criar projeto → importar mídia → montar 30 segundos → animar um texto → salvar/reabrir → exportar MP4**.

O briefing inicial pode usar um vídeo explicativo não factual, com seis planos, uma narração importada e identidade visual própria. Isso exercita montagem, animação e áudio sem depender de uma integração de IA para comprovar a fundação.

### Fila inicial de trabalho

- [ ] Especificar o piloto e seus critérios de aceitação.
- [ ] Desenhar a área de edição e o fluxo de recuperação.
- [ ] Validar motor de prévia/exportação e distribuição no Windows.
- [ ] Registrar decisões de arquitetura e formato de tempo.
- [ ] Criar o modelo mínimo de projeto e comandos de edição.
- [ ] Implementar persistência segura e reabertura.
- [ ] Entregar o piloto editável e seu primeiro MP4.

Este documento estabelece a direção. O backlog detalhado será elaborado por etapa, a partir dos resultados reais, mantendo a liberdade criativa como objetivo e a estabilidade como requisito.
