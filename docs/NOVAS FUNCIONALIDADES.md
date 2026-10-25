# PROJETO: EVOLUÇÃO DO STUDIO DARK — MOTOR DE EDIÇÃO AUDIOVISUAL DIRIGIDO POR IA

## ANDAMENTO DA IMPLEMENTAÇÃO — 24/10/2026

Este quadro separa o que foi verificado no código do que continua planejado. O plano completo permanece aberto.

### Rodada atual: trim, velocidade e recorte de vídeo

- Camadas de vídeo v3 aceitam trim de entrada `sourceInSeconds`, velocidade `speed` de 0,25× a 4×, saída `durationFrames` na timeline e `crop` normalizado da fonte.
- O compositor aplica a mesma sequência de filtros FFmpeg no preview, still e MP4. O inspetor ganhou controles para início da fonte, velocidade, origem e tamanho do recorte; o campo normal de duração continua ajustando a saída na timeline.
- Projetos com trim, velocidade ou crop personalizado usam schema 10; os projetos antigos de vídeo schema 5 continuam abrindo com início zero, velocidade normal e corte central.
- O guia e `capabilities-v1.json` agora descrevem esses campos, limites, defaults e o schema 10.
- Verificação desta rodada: `scripts/build.ps1 -Test` passou (4/4); `scripts/smoke.ps1` passou; `scripts/smoke-production.ps1` passou com voz Microsoft Maria Desktop. O smoke renderizou um clipe com trim, velocidade 1,5× e recorte, comparando preview e MP4 em 32,99 dB. O exemplo Director's Cut também passou, com 2700 quadros e comparação preview/export em 42,61 dB.

### Entrega anterior: efeitos sonoros disparados por Visual Beat

- A ação `sound` usa um arquivo local e ganho de 0–4 para criar uma pista de áudio editável sincronizada ao beat. Projetos schema 9 validam a associação e a timeline preserva o vínculo ao mover, cortar e excluir a pista.
- O Director's Cut de 90 segundos tem 20 beats e inclui `examples/media/soft-chime.wav`; o guia embute a receita e explica que o arquivo local precisa acompanhar o JSON.
- A compilação, 4/4 testes e smoke de produção passaram; a comparação preview/export marcou 42,61 dB.

### Sincronização visual por palavra concluída nesta rodada

- Legendas geradas a partir da narração agora guardam intervalos de palavra resolvidos pelos eventos reais do sintetizador pt-BR. A palavra ativa usa a cor primária da direção de arte e o mesmo compositor desenha prévia e exportação.
- Projetos schema 7 persistem esses intervalos; schemas 1–6 continuam aceitos e sem destaque sincronizado. Se duas palavras começarem no mesmo quadro de vídeo, o quadro destaca a última palavra daquele intervalo, devido à resolução temporal do vídeo.
- O teste automatizado cobre a resolução de oito palavras, salvamento/reabertura do projeto e mudança visual entre quadros de palavra.

### Camadas de vídeo implementadas nesta rodada

- O compositor compartilhado entre prévia e render agora decodifica quadros de vídeo local sob demanda usando o FFmpeg configurado pelo aplicativo. A imagem mantém proporção e preenche sua caixa com corte central.
- O editor oferece **Adicionar vídeo à composição** e reconhece MP4, MOV, MKV, WebM, AVI e M4V. A camada aparece como objeto selecionável e usa os controles de duração, posição e escala existentes.
- Projetos schema 5 persistem a camada sem alterar a leitura dos projetos anteriores. Receitas v3 aceitam `kind: "video"` com `source` local relativo ao JSON ou absoluto; links remotos não são aceitos.
- Guia IA, catálogo de capacidades e README descrevem formatos, comportamento e dependência de arquivo local/FFmpeg. Teste automatizado gera um clipe curto e verifica persistência e diferença entre quadros consecutivos.
- Limites atuais: o áudio embutido no clipe não entra automaticamente na mixagem; importe-o como uma pista independente. Trim de entrada, velocidade, duração de saída e recorte manual já são editáveis. Ainda não há waveform da fonte; em prévia, buscar para trás reinicia a decodificação do clipe.

### Concluído nesta rodada

- O modelo de projeto schema 4 grava e valida shots, Visual Beats e a associação de elementos a seus shots. Projetos anteriores continuam migrando e abrindo.
- Receitas `studio-dark-production` v3 importam várias composições dentro da mesma cena narrativa. O início de cada shot e beat usa tempos medidos da fala (`cueProgress`/`progress`) ou `offsetSeconds`; a duração do shot termina no próximo corte ou no fim da cena.
- Cortes secos trocam composição e câmera por shot. Beats `show`, `hide`, `transform` e `camera` são compilados em keyframes editáveis. A validação rejeita sequência com lacunas, sobreposições e beats fora do shot.
- A timeline exibe os shots e os marcadores dos Visual Beats, permite navegar até seus quadros e arrastar cortes. Um arraste atualiza durações, elementos, beats e câmera em uma só ação de desfazer.
- O inspetor de câmera grava keyframes vinculados ao shot ativo; keyframes acompanham o shot ao arrastar o corte.
- O exemplo `examples/shots-visual-beats-v3.production.json` foi sintetizado com voz pt-BR, compilado e renderizado em MP4; os quadros de shot/beat da exportação foram comparados com a prévia.
- `GUIA-PARA-IA.md` e `capabilities-v1.json` são gerados a partir das capacidades do aplicativo e identificam receita 3/projeto 4, limites e restrições reais.

### Implementado nesta continuação

- Câmera de um shot pode enquadrar automaticamente um elemento ou grupo usando `camera.focus.target` e uma margem `padding` configurável. Os limites do elemento são transformados através dos grupos ancestrais para calcular centro e zoom.
- Com `camera.focus.follow: true`, o compilador recalcula o enquadramento em todos os quadros do shot e grava os estados como keyframes editáveis. O modo não pode ser combinado com Visual Beats de câmera.
- Receitas v3 podem definir `editing.intensity` de 0 a 100. O valor determina movimento/timing de entrada e saída apenas para propriedades não informadas; instruções explícitas da IA e receitas sem esse campo preservam o comportamento anterior.
- Shots v3 aceitam `cut` ou transições `fade`, `dissolve`, `slide` e `zoom`, com duração de 2 a 300 quadros e direção para slide. O compositor sobrepõe as imagens na prévia e na exportação, a timeline marca a faixa entrante e o inspetor edita tipo/duração/direção do shot atual.
- Tipo, direção e duração de transição são salvos no projeto schema 4; projetos anteriores continuam assumindo `cut` sem duração.
- O foco é validado durante a importação: o alvo precisa existir dentro do mesmo shot; a margem aceita valores de 1 a 4. O foco define centro e zoom, preservando a rotação informada.
- O guia da IA, o manifesto de capacidades e a receita de demonstração descrevem auto reframe contínuo e intensidade 50.
- A compilação C++20 e os 4 testes automatizados passaram. O smoke de produção passou com voz Microsoft Maria Desktop: 6 cenas, 1517 quadros, exemplo v3 com 3 shots/3 beats, acompanhamento contínuo de câmera, intensidade 50 e dissolve entre shots; comparação preview/export em 43,82 dB.

### Direção de arte global implementada parcialmente

- Receitas v3 aceitam `artDirection` para paleta global, fonte instalada, tamanhos de título/texto/legenda, posição das legendas e margem segura.
- A receita fornece defaults a elementos sem estilo explícito; `color`, `secondaryColor` e `fontSize` explícitos permanecem preservados. Fundo, cabeçalhos gerados e legendas usam o estilo global.
- Projetos schema 2–7 salvam a direção e mantêm defaults compatíveis ao carregar arquivos antigos. O compositor compartilhado aplica fundo, fonte, cartão e destaque de palavra na legenda em prévia/exportação.
- O inspetor permite alterar cores, fonte instalada, tamanhos de título/texto/legenda, margem segura e posição da legenda; as alterações validam e entram no histórico de undo.
- Ainda faltam estilos por cena/shot, carregar fontes externas e pré-visualização de tema por paleta.
- Validação desta etapa: compilação e quatro testes automatizados passaram; os smokes de app, workspace e produção passaram. A produção sintetizou voz pt-BR, gerou 6 cenas/1517 quadros, e o exemplo v3 de 3 shots/3 beats teve comparação prévia/exportação de 43,82 dB.

### Sincronização por palavra implementada parcialmente

- A síntese Windows pt-BR recolhe eventos `SpeakProgress` com palavra, posição no texto e tempo do fluxo de áudio; o worker registra esses dados junto aos arquivos WAV.
- Receita v3 aceita `wordIndex` no início de shot, no início de elemento, em keyframes e no tempo de Visual Beat, em base zero e ligado ao `cue` da mesma cena. O compilador resolve o evento em quadros e grava os estados no projeto editável.
- Índices sem alinhamento real são rejeitados. Quando o plano de duração fixa acelera uma fala, as posições acompanham o mesmo fator temporal.
- Os eventos SAPI podem ficar ligeiramente fora do tamanho final do WAV devido à diferença entre o relógio reportado e o arquivo; o worker calibra a sequência para o intervalo audível real, preservando a ordem e deixando espaço para o fim da última palavra.
- Os testes cobrem receita, timestamp e resolução em shot, beat, elemento e keyframe. O exemplo v3 usa `wordIndex`; o smoke de produção passou com voz Microsoft Maria Desktop e renderizou os três shots e três beats, incluindo os eventos acionados por palavra. Prévia/exportação do exemplo v3: 43,82 dB.
- Legendas de narração têm destaque da palavra falada pela cor primária; seus tempos ficam gravados no schema 7. A camada de legenda usa layout por métricas de fonte e respeita quebra de linha.
- Ainda faltam edição manual do alinhamento depois da compilação e estilos de destaque configuráveis. A importação de uma produção precisa sintetizar a mesma fala com a voz local para obter tempos confiáveis.

### Controle de intensidade no editor

- O inspetor aplica um perfil de intensidade 0–100 aos elementos visuais da cena atual, calculando tipo/duração de entrada e saída com a mesma escala adotada pela receita v3.
- O comando entra no histórico como uma edição única e pode ser desfeito/refeito. Intensidade zero desliga movimentos automáticos; camadas de vídeo e grupos não são modificados.
- Na receita, a intensidade continua sendo a forma de configurar o perfil automaticamente antes da compilação. No editor, a aplicação é explícita para não substituir movimentos sem ação do usuário.

### Áudio em múltiplas pistas implementado parcialmente

- Projetos schema 4–10 salvam até 32 faixas independentes com caminho local, início/duração em frames, ganho, mute, ducking e automação configurável. Fades exigem schema 6 ou superior; automação de ganho exige schema 8. Os arquivos antigos continuam carregando sem migração destrutiva.
- A timeline importa, remove, silencia, reposiciona, altera duração/ganho, liga ducking e permite definir fades lineares de entrada/saída. Na prévia, cada pista toca sincronizada com o playhead e segue os fades.
- A prévia e a exportação calculam a mesma envoltória RMS suavizada por quadro quando a referência é WAV PCM mono a 24 kHz; a exportação aplica o envelope determinístico na pista. Referências em outros formatos usam sidechaincompress do FFmpeg como fallback.
- Receitas v3 podem declarar pistas por cena com fonte local, sincronizar início por cue/progresso/wordIndex e definir `fadeInFrames`/`fadeOutFrames` até 1800 quadros cada, limitados ao intervalo da pista. Projetos com fades usam schema 6; os fades também são aplicados no render por FFmpeg.
- O smoke compara mix sem/com ducking: RMS durante a voz caiu de 0,0414 para 0,0380, e após a fala os dois ficaram em 0,0324 e 0,0321. Na última verificação estéreo, os fades mediram 0,0045 no começo e 0,0050 no fim contra 0,0229 no trecho estável; automação por pontos/easing mediu 0,0025 no início e 0,0226 após a rampa.

### Situação por fase

| Fase | Situação | O que falta para fechar |
|---|---|---|
| 1 — Fundação | Concluída | Teste temporal cobre 20 cenas, 120 shots, 240 beats concorrentes e cortes de borda; smoke de produção valida receita v3 e projeto editável. |
| 2 — Edição dinâmica | Parcialmente concluída | Auto reframe contínuo, intensidade na receita/editor e transições fade/dissolve/slide/zoom editáveis estão implementados. Falta ritmo guiado por conteúdo, presets de câmera e edição avançada de sobreposição/duração total. |
| 3 — Acabamento audiovisual | Parcialmente concluída | Direção de arte global, pistas com ducking/fades, automação por pontos/easing e efeitos sonoros disparados por Visual Beat estão implementados. Faltam estilos por cena/shot, fontes externas, presets visuais e tipografia animada. |
| 4 — Recursos avançados | Parcialmente concluída | Sincronização por palavra cobre shots, beats, elementos, keyframes e destaque nas legendas; vídeo/B-roll local com trim, velocidade e recorte entra na composição/render. Faltam edição posterior de alinhamento, timeline avançada, áudio embutido nos vídeos e outros formatos de tela. |

### Limites confirmados nesta versão

- `transitionIn` aceita a string legada `cut` ou um objeto com `type`, `durationFrames` e, para slide, `direction`. O inspetor permite ajustar tipo, duração e direção no shot atual. A sobreposição é visual e não prolonga a duração do áudio/cena; o shot de saída congela no último quadro durante a transição. Fade atravessa preto intencionalmente.
- Beats aceitam `show`, `hide`, `transform`, `camera` e `sound`. Um beat `sound` cria uma pista local de uma execução, vinculada ao quadro sincronizado; a fonte não é copiada para o JSON e precisa continuar acessível no computador.
- Shot, Visual Beat, início de elemento, keyframe e destaque de legenda podem usar `wordIndex`, resolvido pelos eventos SAPI da voz local e guardado como quadros/intervalos no projeto. Ainda não há editor manual de alinhamento fonético reutilizável após a importação.
- A timeline mostra e permite navegar pelos shots/beats; as alças movem o corte com ajuste dos conteúdos e um único undo. O inspetor grava câmera no shot ativo, edita a transição de entrada e preserva os keyframes ao mover o corte.
- Vídeo local como camada já existe, com quadros sincronizados à sequência. Áudio próprio do vídeo não é importado automaticamente e mídias remotas não são baixadas. Áudio em múltiplas pistas/ducking, direção de arte global e controles de intensidade já existem; continuam faltando recursos avançados descritos na tabela de fases.
- Auto reframe por ID pode acompanhar o alvo em todos os quadros do shot usando `follow:true`. Esse modo gera keyframes de câmera e é incompatível com Visual Beats de câmera no mesmo shot; presets reutilizáveis e trajetórias Bézier continuam pendentes.
- `editing.intensity` só orienta entradas/saídas automáticas quando a receita omite esses campos. No editor, o comando aplica o perfil explicitamente à cena atual. Nenhum dos modos gera cortes ou efeitos a partir do significado da fala.

### Próxima entrega recomendada

Adicionar direção de arte por cena/shot, preservando a herança global e os estilos explícitos por elemento. Em seguida, avançar para tipografia animada por palavra e seleção múltipla/envelopes da timeline. Áudio embutido em vídeo e formatos de tela adicionais seguem como pendências da fase 4.

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

**Já implementado:** quando há timestamps reais da voz pt-BR, o compositor destaca a palavra correspondente por quadro e salva o alinhamento no projeto schema 7. Sem alinhamento real, mantém a legenda estática. Permanecem pendentes animações tipográficas por palavra com escala/movimento, presets de texto e edição manual do alinhamento.

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

1. [x] Analisar e documentar a arquitetura atual.
2. [x] Criar cenas, shots independentes e Visual Beats.
3. [x] Resolver um modelo temporal por quadros, com validação de intervalos.
4. [x] Sincronizar shots com a fala sintetizada e com `wordIndex` real.
5. [x] Implementar cortes entre shots.
6. [x] Integrar shots/beats à timeline, inspetor e histórico de desfazer.
7. [x] Preservar a leitura de receitas e projetos anteriores.
8. [x] Ampliar testes para produções longas com muitos shots, cortes no limite e eventos concorrentes.

Resultado esperado: conseguir criar várias composições e cortes dentro de uma mesma cena narrativa.

## FASE 2 — EDIÇÃO DINÂMICA

Implementar:

1. [x] Motor de edição dinâmica com intensidade configurável na receita e no editor.
2. [x] Auto Reframe por alvo, inclusive acompanhamento quadro a quadro.
3. [x] Transições fade, dissolve, slide e zoom editáveis.
4. [x] Sincronização temporal de animações com fala/progresso/palavra.
5. [ ] Cortes e ritmo escolhidos automaticamente com base no conteúdo da narração.
6. [ ] Presets reutilizáveis de câmera e edição avançada de duração/overlap da sequência.

Resultado esperado: criar vídeos com ritmo de edição profissional usando os elementos já existentes.

## FASE 3 — ACABAMENTO AUDIOVISUAL

Implementar:

1. [ ] Tipografia animada com presets de movimento/escala e controle por palavra.
2. [x] Destaque de palavra sincronizado quando existe alinhamento real de voz.
3. [x] Efeitos sonoros disparados por Visual Beat.
4. [x] Trilhas importadas em múltiplas pistas, com fades e ducking.
5. [x] Direção de arte global com paleta, fonte instalada e parâmetros de legenda.
6. [ ] Estilos/presets por cena e shot, carregamento de fontes externas e presets visuais avançados.
7. [x] Automação de ganho por pontos com easing na receita, projeto, prévia, exportação e editor.

Resultado esperado: produções com identidade visual e sonora consistente.

## FASE 4 — RECURSOS AVANÇADOS

Implementar:

1. [x] Vídeo local como camada visual editável.
2. [x] B-roll local decodificado na prévia e exportação.
3. [x] Alinhamento real de palavra em shots, beats, elementos, keyframes e legendas.
4. [x] Duração-alvo com aceleração segura e preservação aproximada de tom.
5. [ ] Timeline avançada com envelopes, seleção múltipla e edição mais precisa.
6. [ ] Otimizações de cache/seek para vídeos longos e áudio de preview.
7. [x] Trim de entrada, velocidade, duração de saída e recorte manual de vídeos locais.
8. [ ] Áudio embutido em vídeo e formatos de tela adicionais.

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
- A receita `examples/carros-caros-directors-cut-90s.production.json` gera uma montagem de 90 segundos com narração pt-BR, seis cenas, 18 shots, 20 Visual Beats, um efeito sonoro, câmera, transições e legendas. A exportação contém 2700 quadros; os cortes/beats são comparados à prévia e o projeto abre na interface.

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
