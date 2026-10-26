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

O primeiro recurso premium disponível é `premium:smartphone-premium-01`: use-o quando a história pedir uma tela de produto ou notificação. Combine-o com texto breve, uma imagem local substituível na tela e movimento de câmera sutil. A animação do pacote é discreta e o texto e a imagem continuam editáveis. Esta é uma recomendação artística; os IDs, slots e o JSON aceito ficam no [guia técnico para IA](GUIA-PARA-IA.md). Os outros 29 itens da coleção continuam planejados, conforme [catálogo premium](premium/PREMIUM-CATALOG.md).

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

Dica para travelling: no shot, componha cartões/objetos em posições horizontais progressivas e defina o ponto de partida da câmera centralizado em A. Use beats com `durationFrames` para deslocá-la até B e C. Atenção: o compositor da receita aceita até 200 nós por produção; distribua detalhes de forma intencional.

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
