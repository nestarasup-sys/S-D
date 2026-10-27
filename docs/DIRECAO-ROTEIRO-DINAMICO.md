# Direção de roteiro dinâmico — Studio Dark

Guia criativo complementar ao [GUIA-PARA-IA.md](GUIA-PARA-IA.md). Versão 1.1, outubro de 2026.

## Princípio diretor

A narração informa **o quê**; a direção visual escolhe **como descobrir**. O objetivo é compor um documentário editável que se desenvolve diante do espectador, não uma apresentação de slides. Cada transição, movimento ou destaque precisa carregar informação, hierarquia, emoção ou mudança de escala.

## Linguagens visuais complementares

1. **Tipografia cinética:** um quadro pode conter apenas texto, formas e fundo escuro. Aplique entrada discreta; realce palavras-chave, sem duplicar legendas completas.
2. **Travelling horizontal/vertical:** distribua uma sequência lógica de elementos na composição (A → B → C). Anime a câmera pelos pontos, mantenha continuidade espacial e evite trajetórias sem revelação.
3. **2.5D simulado:** use grupos e camadas frente/meio/fundo com deslocamentos de intensidade diferente. O motor possui câmera 2D; não trate como 3D real.
4. **Infográfico contextual:** prefira comparação, causa–efeito, linha do tempo, processo ou escala. `bars`, `chart-line`, `chart-pie`, `chart-donut`, `timeline`, `network` e `data-table` não carregam dados reais automaticamente.
5. **Objeto + texto:** combine um objeto nativo ou mídia local com um título curto. O objeto explica a metáfora; o texto fixa a ideia.
6. **Respiro cinematográfico:** reduza elementos e movimentos antes de revelações importantes.

## Ritmo sugerido para vídeos de 30 segundos

Um exemplo de variação, não uma regra de relógio: gancho tipográfico; cena de objeto com profundidade; travelling contínuo; infográfico; pausa visual; conclusão. Misture shots curtos e um plano mais longo. Evite repetir `pop` ou `zoom` em toda frase.

## Recursos premium e escolhas artísticas

Recursos premium disponíveis: `premium:smartphone-premium-01` para planos de produto/notificação; `premium:title-reveal-01` para aberturas com hierarquia editorial e revelação; `premium:comparison-pack-01` para contrastar duas condições sem inventar estatísticas. Use valores placeholders apenas enquanto compõe e substitua-os por informação verificada antes de publicar. Imagem da tela do telefone pode ser substituída; os textos e demais camadas são editáveis depois da inserção. Todos usam layouts próprios em paisagem e retrato e são animados no compositor QPainter. Esta seção contém direção artística: os IDs, slots executáveis e o JSON aceito ficam no [guia técnico para IA](GUIA-PARA-IA.md). Os outros 27 itens continuam planejados, conforme [catálogo premium](premium/PREMIUM-CATALOG.md).

## Biblioteca editorial (72 recursos nativos)

O catálogo agora contém 72 desenhos vetoriais, incluindo 58 recursos novos; o exportador de guia lista os IDs atuais. Famílias sugeridas:

- **Tecnologia:** `chip`, `server`, `notification`, `robot`, `laptop`, `cloud`.
- **Comunicação:** `newspaper`, `chat-bubble`, `calendar`, `eye`, `target`.
- **Dados:** `timeline`, `chart-line`, `network`, `dashboard`, `progress-ring`.
- **Finanças:** `credit-card`, `bank`, `receipt`, `factory`.
- **Ciência:** `atom`, `dna`, `brain`, `satellite`, `globe`.
- **Cenários:** `night-city`, `server-room`, `newsroom`, `laboratory-scene`, `map-room`, `financial-district`, `cyber-grid`, `archive-room`.

**Limitação editorial:** vetores nativos dão variedade e permitem montagem offline, mas não substituem fotografias, B-roll e assets pessoais quando o assunto exigir representação realista.

## Direção de câmera suportada

Use `cameraPresets` para trajetórias reutilizáveis com estados em `offsetSeconds` **somente quando a duração mínima do shot estiver garantida**. Para cortes e revelações presos à fala, prefira `beats` de ação `camera`, com `at: {"cue":0,"cueProgress":0.5}`. Não combine câmera por preset com Visual Beats de câmera no mesmo shot. A câmera interpolada possui `x`, `y`, `zoom`, `rotation` e curvas lineares/ease; não há trajetórias Bézier personalizadas.

Dica para travelling: no shot, componha cartões/objetos em posições horizontais progressivas e defina o ponto de partida da câmera centralizado em A. Use beats com `durationFrames` para deslocá-la até B e C. Não há teto fixo agregado de objetos; expansões de grupos, presets e legendas contam como camadas e aumentam o uso de CPU, RAM e tempo de renderização. Prefira detalhe visual que sirva à narrativa e monitore o custo durante a prévia.

## Dirigir movimento sem repetir a mesma fórmula

- **Slide nas quatro direções:** distribua dois ou mais objetos no espaço da composição e use keyframes de posição para entrar da esquerda, direita, de cima ou de baixo. Respeite os limites do canvas e não faça todos os elementos deslizarem juntos.
- **Movimento de câmera:** use pan para conectar pontos que tenham relação narrativa; use zoom para passar do contexto ao detalhe; rotação deve ser rara e motivada. Um beat de câmera altera o enquadramento de todo o conteúdo do shot.
- **Parallax simulado:** coloque camadas de fundo, meio e frente em grupos diferentes e mova cada grupo com deslocamentos menores/maiores. É composição 2D; não há profundidade ou paralaxe física automática.
- **Visual Beats de revelação:** comece os alvos com `opacity: 0` e dispare `show` por `cueProgress` ou palavra com timestamp real. O objeto aparece no quadro do evento e permanece visível. Use `hide` para retirar e `show` posterior para reapresentar.
- **Visual Beats de transformação:** escolha uma ou duas propriedades com mudança legível e use `durationFrames` para interpolar. Uma mudança curta de posição, escala ou opacidade pode explicar crescimento, contraste ou consequência; evite animar todas as propriedades ao mesmo tempo.
- **Infográficos:** revele primeiro a pergunta ou eixo, depois os valores e por fim a conclusão. Dados nativos são elementos visuais ilustrativos, não uma fonte de números.
- **Tipografia cinética:** alterne título isolado, palavra-chave e texto explicativo. Reserve contraste, tempo de leitura e uma região livre para legendas. Evite repetir o mesmo fade em cada shot.
- **Easing:** `linear` serve a deslocamentos constantes; `ease-in` acelera; `ease-out` desacelera; `ease-in-out` suaviza entrada e parada. A curva da keyframe de destino é aplicada ao trecho que termina nela.
- **Premium:** use o Title Reveal para abertura tipográfica, Smartphone para produto/notificação e Comparison Pack para duas condições. Mude enquadramento, escala e ritmo ao alternar templates; não os use como cartões genéricos em sequência.

Para produzir variedade, escreva primeiro o que o espectador deve perceber em cada beat. Faça o movimento responder a essa informação. Intercale composições largas, detalhes, planos tipográficos e pausas. Deixe alguns elementos imóveis enquanto outros se movem. Um shot pode permanecer estático durante uma frase importante.

## As cinco operações temporais

- `transitionIn` define como o novo shot substitui/combina visualmente com o shot anterior; não anima os elementos por conta própria.
- `motion` é a animação padrão de entrada/saída de um elemento.
- `keyframes` armazenam valores editáveis de posição, rotação, escala e opacidade ao longo do tempo.
- `beats` são comandos pontuais sincronizados à fala: mostram/ocultam, transformam um alvo, movem a câmera ou disparam som.
- `camera` enquadra todos os elementos no espaço da composição. Texto comum e legendas também acompanham pan, zoom e rotação; não existe camada de texto fixa no viewport. Planeje o enquadramento para manter títulos legíveis junto ao conteúdo.

Os exemplos completos e aceitos pelo importador estão em [revelação sequencial](../examples/visual-beats-sequential-reveal-v3.production.json) e [transformação + câmera](../examples/visual-beats-transform-camera-v3.production.json). O guia técnico define a sintaxe e a sincronização real pela voz.

## Sincronização e durações

- Divida a voz em trechos `narration` faláveis, máximo 150 caracteres cada.
- Posicione shots, beats e keyframes usando `cueProgress` (0 a 1) ou `wordIndex` com alinhamento real do sintetizador.
- Não invente timestamps para palavras ou duração exata da voz antes do WAV.
- `targetDurationSeconds` é meta. O compilador pode ampliar duração se a voz exigir.
- `transitionIn` aceita `cut`, `fade`, `dissolve`, `slide` e `zoom`; não existe transição arbitrária por máscara no contrato atual.

## Critérios de qualidade

Antes de produzir, pergunte: cada shot acrescenta uma ideia? O movimento responde à narração? A composição preserva as legendas? Houve alternância entre texto puro, objeto, profundidade e dado visual? Nenhum elemento é apresentado como evidência ou estatística não verificada? Os IDs pertencem ao catálogo?

## Exemplo mínimo: objeto nativo

```json
{
  "id": "chip",
  "kind": "native",
  "asset": "chip",
  "x": 410, "y": 180,
  "width": 320, "height": 240,
  "color": "#55E2C4"
}
```

Para imagens e vídeos externos, adicione arquivos locais à biblioteca pessoal e use seus `assetId` **reais**, ou caminhos existentes com `source`. O app não baixa mídia automaticamente.
