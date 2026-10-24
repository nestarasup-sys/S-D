# PROJETO: EVOLUÇÃO DO STUDIO DARK — MOTOR DE EDIÇÃO AUDIOVISUAL DIRIGIDO POR IA

## ANDAMENTO DA IMPLEMENTAÇÃO — 24/10/2026

Este quadro separa o que foi verificado no código do que continua planejado. O plano completo permanece aberto.

### Concluído nesta rodada

- O modelo de projeto schema 4 grava e valida shots, Visual Beats e a associação de elementos a seus shots. Projetos anteriores continuam migrando e abrindo.
- Receitas `studio-dark-production` v3 importam várias composições dentro da mesma cena narrativa. O início de cada shot e beat usa tempos medidos da fala (`cueProgress`/`progress`) ou `offsetSeconds`; a duração do shot termina no próximo corte ou no fim da cena.
- Cortes secos trocam composição e câmera por shot. Beats `show`, `hide`, `transform` e `camera` são compilados em keyframes editáveis. A validação rejeita sequência com lacunas, sobreposições e beats fora do shot.
- A timeline exibe os shots e os marcadores dos Visual Beats, permite navegar até seus quadros e arrastar cortes. Um arraste atualiza durações, elementos, beats e câmera em uma só ação de desfazer.
- O exemplo `examples/shots-visual-beats-v3.production.json` foi sintetizado com voz pt-BR, compilado e renderizado em MP4; os quadros de shot/beat da exportação foram comparados com a prévia.
- `GUIA-PARA-IA.md` e `capabilities-v1.json` são gerados a partir das capacidades do aplicativo e identificam receita 3/projeto 4, limites e restrições reais.

### Situação por fase

| Fase | Situação | O que falta para fechar |
|---|---|---|
| 1 — Fundação | Parcialmente concluída | Controles de câmera por shot no inspetor; ampliar cenários automatizados de regressão para composição visual e movimentos de câmera. |
| 2 — Edição dinâmica | Pendente | Dynamic Edit Engine configurável, intensidade 0–100, auto reframe/foco em elemento e transições além do corte seco. |
| 3 — Acabamento audiovisual | Pendente | Tipografia animada e direção de arte global, presets visuais avançados, efeitos por beat e trilhas importadas com mixagem/ducking. |
| 4 — Recursos avançados | Pendente | Vídeo/B-roll como camada, avaliação local de alinhamento por palavra em pt-BR, timeline avançada e outros formatos de tela. |

### Limites confirmados nesta versão

- `transitionIn` aceita somente `cut`; não há sobreposição de shots ou dissolve/fade/slide/zoom.
- Beats aceitam `show`, `hide`, `transform` e `camera`. Não disparam áudio e não destacam palavras.
- Os cortes e beats são resolvidos por fala/progresso proporcional depois da síntese. Não existe timestamp por palavra.
- A timeline mostra e permite navegar pelos shots/beats; as alças movem o corte com ajuste dos conteúdos e um único undo. Câmera de shot ainda não tem controles diretos no inspetor.
- Vídeo como camada, mídia remota automática, áudio em múltiplas pistas, direção de arte global e intensidade de edição ainda não existem.

### Próxima entrega recomendada

Fechar os controles de câmera por shot no inspetor e ampliar a validação visual automatizada. Depois iniciar a fase 2 com intensidade explícita e transições, cada qual coberta por testes de render e comparação entre prévia e exportação.

---

## 1. OBJETIVO PRINCIPAL

Quero transformar o Studio Dark em uma ferramenta avançada de criação e edição automática de vídeos dirigidos por inteligência artificial.

Atualmente, o aplicativo permite importar produções JSON, gerar narração local, criar cenas com elementos nativos, aplicar movimentos, keyframes, grupos, câmera e exportar vídeos.

Essa estrutura deve ser preservada e evoluída.

**Meu objetivo é permitir que uma IA externa, como o ChatGPT, funcione como diretora, roteirista, animadora e editora audiovisual.**

A IA deve conseguir produzir um arquivo JSON com instruções detalhadas sobre:

- O que deve aparecer na tela.
- Quando cada elemento deve aparecer e desaparecer.
- Quando a câmera deve se movimentar.
- Quando deve ocorrer um corte.
- Como devem funcionar as transições.
- Como as palavras devem ser destacadas.
- Como os objetos devem ser animados.
- Quando efeitos sonoros devem acontecer.
- Como os elementos devem acompanhar a narração.
- Qual deve ser a duração de cada plano.
- Qual deve ser a duração total do vídeo.
- Qual estilo visual deve ser utilizado.
- Qual ritmo de edição deve ser aplicado.

O Studio Dark será responsável por interpretar essas instruções, produzir a narração, sincronizar todos os elementos e montar o vídeo automaticamente.

**Não quero apenas mais animações. Quero um verdadeiro motor de montagem audiovisual programável por JSON.**

---

## 2. REGRAS IMPORTANTES DE DESENVOLVIMENTO

Antes de implementar qualquer modificação:

1. Analise a arquitetura atual do Studio Dark.
2. Identifique os módulos responsáveis por importação JSON, síntese de voz, sincronização, renderização, câmera, grupos, animações, timeline e exportação.
3. Identifique o que já existe, o que pode ser reutilizado e o que precisa ser desenvolvido.
4. Preserve os recursos existentes, incluindo projetos e receitas compatíveis.
5. Evite reescrever o aplicativo inteiro sem necessidade.
6. Implemente progressivamente, mantendo o aplicativo funcional.
7. Crie testes para os recursos novos e para evitar regressões.
8. Não apresente funcionalidades como concluídas se ainda não funcionarem.

As novas funções devem ser integradas ao editor existente e, sempre que tecnicamente possível, permanecer editáveis pelo usuário.

Não quero apenas exemplos fictícios de implementação, pseudocódigo ou documentação. Quero as funcionalidades realmente implementadas no projeto.

Se alguma função depender de bibliotecas adicionais, avalie a compatibilidade com Windows e com a tecnologia já utilizada pelo aplicativo.

---

# PARTE 1 — NOVA ARQUITETURA DE MONTAGEM

## 3. CRIAR UM SISTEMA DE SHOTS (PLANOS VISUAIS)

Esta é uma das mudanças mais importantes.

Atualmente, o JSON organiza a produção principalmente em cenas contendo narração, elementos e câmera.

Quero adicionar o conceito de **plano visual (shot)**.

Uma cena narrativa poderá conter vários planos visuais independentes.

Exemplo:

Uma cena de 12 segundos apresenta um carro à venda.

Durante esses 12 segundos, quero que o aplicativo consiga executar:

- Plano 1: carro ocupando a tela inteira.
- Plano 2: corte para o preço anunciado.
- Plano 3: aproximação da expressão do personagem.
- Plano 4: corte para documentos e despesas.

Tudo isso deve acontecer dentro de uma única cena narrativa, sem precisar interromper ou duplicar a narração.

Cada shot poderá ter:

- Identificador único.
- Duração planejada ou relativa.
- Início e término.
- Composição visual própria.
- Elementos e grupos.
- Animações.
- Câmera independente.
- Transição de entrada e saída.
- Relação com a narração.
- Efeitos sonoros associados.
- Instruções visuais de direção.

Os planos devem permitir cortes secos, mudanças de enquadramento e transições.

A IA deve conseguir planejar a montagem com precisão.

**Importante:** a implementação não deve tratar cada shot como uma nova cena obrigatória. Uma cena pode conter diversos shots sem interromper a fala.

---

## 4. CRIAR VISUAL BEATS

Quero implementar um sistema chamado **Visual Beats**.

Um Visual Beat representa uma pequena ação visual sincronizada com um momento da narração.

Exemplo:

Narração:

"Lucas encontrou um carro barato. Mas havia um problema escondido."

Visual Beats:

1. O carro aparece.
2. O preço entra em destaque.
3. A câmera se aproxima do carro.
4. O preço desaparece.
5. Um aviso aparece na tela.
6. A câmera muda para o personagem preocupado.

Esses eventos devem ser organizados dentro dos shots.

Cada beat poderá controlar:

- Aparição de objetos.
- Desaparecimento de objetos.
- Movimentação.
- Mudança de escala.
- Opacidade.
- Rotação.
- Destaque de texto.
- Mudança de câmera.
- Corte visual.
- Transição.
- Disparo de efeito sonoro.

Quero poder controlar esses eventos por JSON.

Os Visual Beats devem permitir construir vídeos com novidades visuais frequentes, sem precisar criar dezenas de cenas narrativas.

---

# PARTE 2 — SINCRONIZAÇÃO PRECISA COM A NARRAÇÃO

## 5. CRIAR UM MOTOR INTELIGENTE DE SINCRONIZAÇÃO

Atualmente, o Studio Dark utiliza cues e mede a duração real da voz gerada.

Quero evoluir essa arquitetura para permitir que a IA planeje a duração dos vídeos e que o aplicativo tente cumprir esse planejamento.

Implementar dois modos:

### Modo A — Duração natural

O aplicativo:

1. Recebe o roteiro.
2. Gera a narração.
3. Mede a duração real das falas.
4. Organiza os shots e Visual Beats.
5. Sincroniza a montagem com a voz gerada.

### Modo B — Duração-alvo

A IA informa uma duração desejada, por exemplo 90 segundos.

O aplicativo deverá:

1. Receber o roteiro e o planejamento de tempo.
2. Gerar os áudios.
3. Medir a duração real.
4. Comparar a duração gerada com a desejada.
5. Distribuir o tempo entre falas, pausas, cenas e shots.
6. Ajustar a velocidade da voz dentro de limites configuráveis.
7. Preservar o tom da voz, evitando alterações artificiais de pitch.
8. Adaptar as animações aos tempos resolvidos.
9. Sincronizar legendas e efeitos.
10. Informar quando a duração solicitada for inviável sem prejudicar a naturalidade.

A duração-alvo não deve forçar uma narração absurdamente rápida.

Quero que o aplicativo utilize um relógio de referência único para áudio, vídeo e animações, com conversão consistente para frames de exportação.

A duração final deve ser calculada a partir dos dados reais da síntese, não de timestamps imaginados pela IA.

---

## 6. CRIAR SINCRONIZAÇÃO PROPORCIONAL

Além do `cue` e do `offsetSeconds` existentes, quero um sistema de eventos por porcentagem da fala.

Exemplo conceitual:

- 0%: o carro aparece.
- 25%: a câmera começa a aproximar.
- 50%: aparece o preço.
- 75%: o personagem reage.
- 100%: o plano termina.

Se a fala durar 4 segundos ou 6 segundos, os eventos deverão se adaptar proporcionalmente.

O nome definitivo dos campos JSON deverá ser escolhido durante a implementação e documentado claramente.

Os recursos atuais de `cue`, `offsetSeconds`, `keyframes` e `easing` precisam continuar funcionando.

---

## 7. SINCRONIZAÇÃO POR PALAVRA

Quero também estudar e, se viável, implementar alinhamento entre texto e áudio.

Exemplo:

Narração:

"Mas o verdadeiro problema estava no preço."

Quando a voz pronunciar "problema", o aplicativo poderá:

- Aplicar um zoom.
- Destacar a palavra na legenda.
- Exibir uma informação.
- Executar um efeito sonoro.

Isso exige identificar o momento real das palavras no áudio.

Não quero que o aplicativo adivinhe esses tempos contando caracteres.

Avalie uma solução de alinhamento de fala que funcione em português brasileiro, preferencialmente localmente e compatível com Windows.

Se não for possível obter um alinhamento confiável, mantenha a sincronização por fala e não simule uma precisão inexistente.

---

# PARTE 3 — MOTOR DE EDIÇÃO DINÂMICA

## 8. IMPLEMENTAR DYNAMIC EDIT ENGINE

Quero um sistema capaz de interpretar as instruções de edição fornecidas pela IA.

Esse motor deve controlar:

- Cortes.
- Duração dos planos.
- Enquadramentos.
- Movimentação de câmera.
- Revelações de elementos.
- Transições.
- Efeitos visuais.
- Tipografia.
- Ritmo geral do vídeo.

Quero que seja possível criar vídeos com dinâmica semelhante à de conteúdos modernos de YouTube, vídeos explicativos e documentários.

Não quero que o vídeo fique visualmente parado por longos períodos sem intenção narrativa.

Entretanto, não quero um sistema que aplique cortes aleatórios a cada dois segundos.

**A edição deve obedecer ao significado da narração e ao planejamento enviado pela IA.**

Como referência inicial:

- Mudanças visuais leves a cada 2–4 segundos.
- Mudanças de composição em intervalos de 5–8 segundos.
- Mudanças narrativas em intervalos maiores, conforme a história.

Esses valores devem ser configuráveis, não obrigatórios.

Momentos dramáticos podem ter planos longos.

---

## 9. CONTROLE DE INTENSIDADE DE EDIÇÃO

Quero que a IA possa definir a intensidade da edição.

Criar um parâmetro configurável de 0 a 100 ou uma configuração equivalente.

Exemplos:

**0–30: Cinematográfico**

- Poucos cortes.
- Movimentos suaves.
- Transições discretas.
- Composições mais contemplativas.

**31–70: Dinâmico**

- Cortes frequentes, mas justificados.
- Destaques visuais.
- Zooms moderados.
- Movimentação de objetos.
- Revelações progressivas.

**71–100: Acelerado**

- Cortes mais rápidos.
- Mudanças frequentes de enquadramento.
- Tipografia expressiva.
- Efeitos visuais pontuais.
- Maior energia audiovisual.

O usuário também deverá conseguir alterar esse valor pelo editor, quando possível.

A intensidade deve orientar o comportamento automático, mas não substituir instruções explícitas da IA.

---

## 10. AUTO REFRAME E CÂMERA AVANÇADA

Quero evoluir o sistema de câmera existente.

Além dos keyframes atuais, permitir:

- Enquadramentos independentes por shot.
- Cortes instantâneos de câmera.
- Zoom-in e zoom-out.
- Movimentos de pan.
- Movimentos suaves entre posições.
- Foco em um elemento ou grupo.
- Diferentes curvas de interpolação.
- Presets de movimentação reutilizáveis.

Exemplo:

A IA pode solicitar:

"Aproxime a câmera do preço enquanto a narração menciona o valor."

O JSON deve representar essa intenção com parâmetros objetivos.

O motor deve calcular a transformação de câmera correspondente e executar o movimento corretamente.

Manter a compatibilidade com as curvas de easing já existentes.

A implementação precisa lidar com coordenadas locais e globais de grupos aninhados, evitando focos incorretos.

---

## 11. TRANSIÇÕES INTELIGENTES

Quero um sistema de transições entre shots.

Implementar inicialmente:

- Corte seco.
- Fade.
- Dissolve.
- Slide horizontal.
- Slide vertical.
- Zoom transition.

As transições devem ter duração configurável.

Preciso poder definir a transição pelo JSON e visualizar seu resultado no editor.

O aplicativo deve tratar corretamente a sobreposição temporal de planos durante transições e não produzir intervalos pretos indesejados.

Evite transições excessivamente chamativas como padrão. Cortes secos devem continuar sendo uma opção importante.

---

# PARTE 4 — EVOLUÇÃO VISUAL

## 12. SUPORTE A VÍDEOS COMO CAMADAS

O guia atual documenta imagens e SVG, mas não vídeo como camada.

Quero adicionar suporte a arquivos de vídeo locais.

A IA poderá combinar:

- Vídeos reais.
- Objetos vetoriais.
- Imagens.
- Personagens.
- Formas.
- Texto.
- Grupos.

Os vídeos devem aceitar, conforme viabilidade técnica:

- Posicionamento.
- Escala.
- Corte e ajuste ao enquadramento.
- Rotação.
- Opacidade.
- Início e término de reprodução.
- Trim.
- Loop.
- Controle de áudio.
- Sobreposição com outros elementos.
- Integração com a timeline.

Quero utilizar vídeos reais como B-roll, misturando-os com animações do Studio Dark.

Priorize suporte a arquivos locais e evite depender de download automático de URLs.

O app deve informar quando um arquivo estiver ausente, incompatível ou corrompido.

---

## 13. BIBLIOTECA VISUAL E PRESETS

Quero preservar e expandir a biblioteca pessoal existente.

O aplicativo já possui suporte a elementos pessoais e presets compostos.

Quero permitir presets reutilizáveis para:

- Aberturas.
- Textos animados.
- Personagens.
- Cartões informativos.
- Comparações.
- Gráficos ilustrativos.
- Transições.
- Composições completas.
- Elementos de destaque.

O guia exportado deverá incluir os identificadores realmente disponíveis na biblioteca pessoal.

A IA não deverá precisar inventar IDs de elementos.

Também quero manter a possibilidade de editar os componentes internos de uma composição importada.

---

## 14. SISTEMA DE TIPOGRAFIA ANIMADA

Quero legendas e textos mais sofisticados.

Funcionalidades desejadas:

- Animação de entrada e saída.
- Destaque de palavras.
- Mudança de cor.
- Mudança de escala.
- Movimento vertical e horizontal.
- Palavras-chave em destaque.
- Presets de títulos.
- Textos grandes de impacto.
- Animação de números e informações.
- Integração com o alinhamento da narração.

Quando o alinhamento por palavra estiver disponível, permitir legendas com destaque progressivo.

Quando não estiver, utilizar os tempos medidos por fala.

Garantir que o texto permaneça legível e não invada regiões importantes da composição.

---

## 15. DIREÇÃO DE ARTE GLOBAL

Quero que a IA possa definir um estilo visual consistente para a produção.

Exemplo de parâmetros:

- Paleta de cores.
- Tipografia.
- Tamanho dos títulos.
- Estilo de legendas.
- Estilo de cartões.
- Estilo de animação.
- Intensidade de movimento.
- Margens de segurança.
- Posição padrão das legendas.
- Tratamento visual dos elementos.

Essas propriedades devem funcionar como padrões globais, podendo ser sobrescritas em cenas ou shots.

A ideia é evitar que cada cena pareça pertencer a um vídeo completamente diferente.

---

# PARTE 5 — ÁUDIO PROFISSIONAL

## 16. MOTOR DE DESIGN SONORO

Quero evoluir o sistema de áudio existente.

Atualmente, o guia documenta narração local, música procedural e sinais de troca de cena.

Quero permitir:

- Trilhas de música importadas.
- Efeitos sonoros locais.
- Efeitos associados aos Visual Beats.
- Controle de volume.
- Fade-in e fade-out.
- Posicionamento temporal.
- Sincronização com cortes e animações.
- Mixagem de múltiplas fontes.
- Redução automática da música durante a narração (ducking).

Exemplo:

Quando uma informação importante aparece, o aplicativo toca um efeito discreto.

Quando a narração começa, a trilha diminui de volume.

Quando a narração termina, o volume pode voltar ao normal.

Quero que a IA possa especificar esses comportamentos no JSON.

Se um efeito não existir na biblioteca, o aplicativo não deve inventar um arquivo.

Os caminhos e os IDs precisam ser verificados.

---

# PARTE 6 — NOVO FORMATO DE PRODUÇÃO JSON

## 17. EVOLUIR O FORMATO DE FORMA ORGANIZADA

Quero que você proponha e implemente uma evolução do formato de produção.

O formato atual utiliza:

- `format: studio-dark-production`
- `version: 2`
- `catalogVersion: 1`

O guia também documenta projeto schema 3.

Avalie criar uma nova versão de produção para os recursos avançados, mantendo suporte às receitas anteriores.

A estrutura deverá conseguir representar conceitualmente:

**Produção**
- Configurações globais.
- Estilo visual.
- Configuração de narração.
- Duração-alvo.
- Intensidade de edição.
- Configurações de áudio.
- Fontes e mídia.

**Cenas**
- Narração.
- Duração planejada.
- Contexto visual.
- Lista de shots.

**Shots**
- Composição.
- Duração.
- Câmera.
- Elementos.
- Transições.
- Animações.

**Visual Beats**
- Evento.
- Referência temporal.
- Elemento afetado.
- Propriedades alteradas.
- Efeito sonoro, quando aplicável.

**Sincronização**
- Tempo por fala.
- Progresso relativo.
- Eventos por palavra, quando suportados.
- Duração final resolvida.

O esquema definitivo deve evitar propriedades redundantes e ambiguidades entre scene, shot, cue e beat.

Defina regras claras de herança, duração, ordem de renderização e resolução de conflitos.

**Não use esses nomes como se já existissem no aplicativo. Eles são requisitos conceituais para o novo design.**

---

# PARTE 7 — EDITOR E VALIDAÇÃO

## 18. TIMELINE EDITÁVEL

Após a importação do JSON, quero que o usuário possa modificar a produção.

Sempre que viável, o editor deverá permitir:

- Ajustar a duração de shots.
- Mover cortes.
- Alterar transições.
- Editar movimentos de câmera.
- Ajustar elementos.
- Alterar texto.
- Modificar volume.
- Ajustar efeitos sonoros.
- Visualizar narração e eventos temporais.
- Editar presets sem destruir sua hierarquia.

Não quero que a produção seja transformada em um vídeo fechado imediatamente.

Ela deve continuar sendo um projeto editável do Studio Dark.

---

## 19. PRÉVIA E VALIDAÇÃO AUTOMÁTICA

Quero um sistema de validação mais robusto.

Antes de montar ou exportar, o aplicativo deverá detectar:

- JSON inválido.
- Campos desconhecidos.
- IDs duplicados.
- Recursos indisponíveis.
- Arquivos ausentes.
- Tempos impossíveis.
- Keyframes fora da duração.
- Eventos fora de shots.
- Animações incompatíveis.
- Transições inválidas.
- Referências a palavras sem alinhamento.
- Incompatibilidades de versões.
- Elementos fora do enquadramento.
- Textos potencialmente ilegíveis.
- Sobreposição com a faixa de legendas.

Os erros devem explicar qual cena, shot, beat ou elemento causou o problema.

Quando possível, ofereça uma prévia da montagem antes da exportação.

A prévia e o vídeo exportado devem utilizar a mesma lógica de sincronização.

---

## 20. EVOLUIR OS LIMITES TÉCNICOS COM SEGURANÇA

O guia atual documenta:

- 1280 × 720.
- 30 FPS.
- Até 20 cenas.
- Até 200 elementos.
- Até 10 trechos de narração por cena.
- Até 150 caracteres por trecho.
- Até 8 níveis de grupos aninhados.

Quero que você avalie se esses limites continuam adequados após a criação de shots e Visual Beats.

Se necessário, implemente uma arquitetura que permita muitos eventos temporais sem criar centenas de elementos simultâneos.

Os limites só devem ser aumentados quando houver suporte real e testes de desempenho.

Considere também a possibilidade futura de projetos verticais em 1080 × 1920, sem prejudicar o formato horizontal.

---

# PARTE 8 — FLUXO FINAL DE PRODUÇÃO

## 21. COMO QUERO QUE O STUDIO DARK FUNCIONE

Este deve ser o fluxo ideal:

**Etapa 1 — Direção pela IA**

O usuário envia um roteiro e o guia atualizado ao ChatGPT.

O ChatGPT desenvolve a direção audiovisual e gera um arquivo JSON com:

- Roteiro preservado.
- Cenas.
- Shots.
- Visual Beats.
- Movimentos de câmera.
- Animações.
- Transições.
- Efeitos.
- Direção de arte.
- Planejamento temporal.

**Etapa 2 — Importação**

O Studio Dark lê o JSON, valida o conteúdo e identifica todos os recursos.

**Etapa 3 — Narração**

O aplicativo sintetiza a voz, mede os áudios e obtém os tempos reais.

**Etapa 4 — Resolução temporal**

O motor combina:

- Duração da voz.
- Duração-alvo.
- Duração dos shots.
- Pausas.
- Eventos temporais.
- Animações.
- Transições.

**Etapa 5 — Montagem**

O Studio Dark constrói o vídeo com todos os elementos sincronizados.

**Etapa 6 — Prévia e edição**

O usuário visualiza o resultado, modifica elementos e ajusta a timeline.

**Etapa 7 — Exportação**

O aplicativo renderiza o vídeo final preservando a sincronização.

Quero que esse fluxo seja o principal objetivo técnico de toda a evolução.

---

# PARTE 9 — ORDEM DE IMPLEMENTAÇÃO

Não tente implementar todos os recursos de uma só vez sem garantir estabilidade.

Trabalhe por etapas.

## FASE 1 — FUNDAÇÃO DO NOVO MOTOR

Prioridade máxima:

1. Analisar a arquitetura atual.
2. Criar a hierarquia de cenas, shots e Visual Beats.
3. Implementar um modelo temporal consistente.
4. Sincronizar os shots com a narração real.
5. Implementar cortes entre shots.
6. Integrar os planos ao editor.
7. Manter compatibilidade com as receitas antigas.

Resultado esperado: conseguir criar várias composições e cortes dentro de uma mesma cena narrativa.

## FASE 2 — EDIÇÃO DINÂMICA

Implementar:

1. Dynamic Edit Engine.
2. Controle de intensidade.
3. Auto Reframe.
4. Transições.
5. Animações proporcionais à fala.
6. Melhorias nos movimentos de câmera.

Resultado esperado: criar vídeos com ritmo de edição profissional usando os elementos já existentes.

## FASE 3 — ACABAMENTO AUDIOVISUAL

Implementar:

1. Tipografia animada.
2. Destaque de palavras quando houver alinhamento.
3. Efeitos sonoros.
4. Trilhas importadas.
5. Ducking.
6. Direção de arte global.
7. Presets avançados.

Resultado esperado: produções com identidade visual e sonora consistente.

## FASE 4 — RECURSOS AVANÇADOS

Implementar:

1. Vídeo como camada.
2. B-roll.
3. Alinhamento preciso de palavras, se viável.
4. Duração-alvo com ajuste seguro da voz.
5. Timeline mais avançada.
6. Otimizações de renderização.
7. Suporte a outros formatos de tela, conforme viabilidade.

Resultado esperado: ampliar as possibilidades criativas para produções mais elaboradas.

Ajuste a ordem se encontrar dependências técnicas importantes.

---

# PARTE 10 — TESTES OBRIGATÓRIOS

## 22. CRIAR UMA PRODUÇÃO REAL DE TESTE

Após implementar cada fase, crie uma produção demonstrativa válida.

Quero um vídeo de aproximadamente 90 segundos contando uma história curta.

Ele deverá testar, conforme as funcionalidades disponíveis:

- Narração em português brasileiro.
- Múltiplos shots na mesma cena.
- Cortes durante a narração.
- Visual Beats.
- Movimentação de câmera.
- Elementos animados.
- Grupos aninhados.
- Transições.
- Textos em destaque.
- Efeitos sonoros.
- Sincronização.
- Exportação.

Use um roteiro que tenha mudanças de composição suficientes para demonstrar os benefícios do novo motor.

**Não basta verificar se o JSON é aceito.**

É necessário testar se:

- O vídeo realmente renderiza.
- A narração permanece sincronizada.
- Os cortes acontecem corretamente.
- As animações não desaparecem prematuramente.
- As transições não causam frames pretos.
- As legendas acompanham o áudio.
- A timeline continua editável.
- O resultado exportado corresponde à prévia.

Adicione testes automatizados sempre que possível, além da validação visual.

---

# PARTE 11 — ATUALIZAÇÃO DO GUIA PARA IA

## 23. ESSA ETAPA É OBRIGATÓRIA

Depois de implementar e testar as funcionalidades, atualize o arquivo **GUIA-PARA-IA.md** gerado pelo Studio Dark.

Esse arquivo será enviado a uma IA externa para que ela produza vídeos usando os recursos existentes.

Portanto, o guia precisa funcionar como uma especificação técnica precisa.

O guia atualizado deverá conter:

### A. Versões e capacidades

- Versão do formato de produção.
- Versão do projeto.
- Versão do catálogo.
- Resolução e FPS.
- Limites técnicos.
- Recursos disponíveis.

### B. Estrutura JSON completa

Documente cada campo aceito:

- Nome.
- Tipo.
- Finalidade.
- Valor padrão.
- Limites.
- Valores válidos.
- Campos obrigatórios.
- Incompatibilidades.

### C. Sincronização

Explique exatamente como a IA deve indicar:

- Duração-alvo.
- Cues.
- Progresso da fala.
- Duração dos shots.
- Eventos por palavra.
- Eventos por tempo.
- Transições.
- Keyframes.

### D. Elementos visuais

Liste todos os recursos realmente disponíveis:

- Tipos de elementos.
- Elementos nativos.
- Grupos.
- Presets.
- Arquivos de imagem.
- Arquivos de vídeo, se implementados.
- Animações.
- Movimentos de câmera.
- Tipografia.
- Efeitos.

### E. Biblioteca pessoal

Inclua somente os IDs válidos exportados pela instalação atual.

Não invente elementos ou arquivos.

### F. Exemplo de produção profissional

Inclua pelo menos um exemplo completo de JSON, válido e testado, demonstrando:

- Múltiplas cenas.
- Múltiplos shots.
- Visual Beats.
- Cortes.
- Movimento de câmera.
- Narração.
- Sincronização.
- Transições.
- Animações.

O exemplo precisa ser importável pelo aplicativo.

### G. Limitações

Liste claramente:

- O que já funciona.
- O que ainda não funciona.
- O que depende de arquivos locais.
- O que exige bibliotecas opcionais.
- O que não é compatível com formatos antigos.

**O guia não pode anunciar funcionalidades planejadas como se já estivessem implementadas.**

---

# PARTE 12 — CRITÉRIO FINAL DE SUCESSO

Meu objetivo final é conseguir fazer o seguinte:

1. Abrir uma conversa com o ChatGPT.
2. Enviar o guia atualizado do Studio Dark.
3. Pedir um vídeo de 1 minuto e 30 segundos sobre determinado assunto.
4. Informar o estilo desejado: dinâmico, cinematográfico, documental ou acelerado.
5. Receber um arquivo JSON completo com a direção audiovisual.
6. Importar o arquivo no Studio Dark.
7. Ter a narração gerada automaticamente.
8. Ter os planos, cortes, animações, legendas e efeitos sincronizados.
9. Visualizar uma montagem bem estruturada.
10. Fazer ajustes no editor e exportar o vídeo.

Quero que a IA consiga tomar decisões criativas de edição, enquanto o Studio Dark executa as instruções de maneira determinística e previsível.

Não quero depender de montar manualmente dezenas de cenas para cada vídeo.

Também não quero que o aplicativo invente decisões importantes contrárias ao planejamento explícito da IA.

O sistema deve combinar **liberdade criativa para a IA com execução técnica precisa pelo aplicativo**.

---

# INSTRUÇÃO FINAL AO DESENVOLVEDOR

Analise o código atual do Studio Dark e utilize este documento como especificação de evolução.

Identifique as alterações necessárias e implemente os recursos progressivamente.

Priorize o sistema de shots, Visual Beats e sincronização temporal, pois essas funcionalidades são a base de todo o restante.

A cada fase:

1. Implemente as alterações no código real.
2. Integre as funcionalidades ao aplicativo.
3. Execute os testes disponíveis.
4. Corrija regressões.
5. Gere uma produção demonstrativa.
6. Informe quais recursos estão totalmente funcionais, parcialmente funcionais ou pendentes.

Ao finalizar as etapas implementadas, atualize o gerador do **GUIA-PARA-IA.md** para documentar fielmente as capacidades reais.

O resultado desejado é transformar o Studio Dark em um sistema capaz de receber a direção de uma IA e produzir vídeos com montagem dinâmica, narração sincronizada, animações, câmera, tipografia, transições e áudio, mantendo a possibilidade de edição manual.

**A prioridade máxima é a qualidade da montagem final, a precisão da sincronização, a estabilidade do aplicativo e a capacidade de uma IA externa controlar a produção por JSON.**

Não quero apenas adicionar funcionalidades isoladas. Quero que todas elas trabalhem juntas como um motor de produção audiovisual coerente.
