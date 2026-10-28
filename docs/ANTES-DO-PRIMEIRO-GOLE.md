# Exemplo de direção · Antes do primeiro gole

`examples/antes-do-primeiro-gole.production.json` é uma receita v3 completa de referência. Ela conduz uma gota da origem ao copo em oito cenas e 24 shots; voz Microsoft Daniel e seus limites de palavras são medidos no Windows pelo fluxo normal do Studio Dark.

## Direção

O arco passa do mistério à origem, ao que não se vê, às barreiras de tratamento, à medição, à rede urbana, às responsabilidades complementares e ao retorno ao copo. Cada cena muda de linguagem: tipografia e uma gota, mapa de bacia, mundo microscópico ilustrativo, fluxograma, laboratório, cidade, identificação editorial e fechamento tipográfico.

Os dois travellings usam `camera.path` espacial em alvos ordenados. Os elementos existem desde o começo no mundo da cena; entram e saem do quadro pelo deslocamento da câmera. Margens amplas evitam aproximar demais elementos pequenos, e SVGs originais mais largos que o canvas cobrem o espaço percorrido. Nos shots que chegam por dissolve, o último quadro de saída permanece visível; a transição é responsável pela passagem, sem o fade automático de oito quadros apagar o plano antes do dissolve. As legendas permanecem ancoradas à tela, enquanto textos comuns continuam acompanhando a câmera.

## Bibliotecas e mídia

A receita personaliza os nove IDs premium disponíveis com seus slots e paletas reais. Usa nativos de mapas, ciência, laboratório, infraestrutura, dados e casas. Question Hook abre a investigação; Comparison Pack separa aparência de análise; Statistic Callout contextualiza “100+ parâmetros”; Number Emphasis identifica uma sequência comum de cinco etapas; Smartphone mostra um painel explicitamente ilustrativo; Lower Third e Source Label atribuem as referências; Dramatic Quote usa uma frase original; Title Reveal abre e fecha a peça.

SVGs, a tela PNG do smartphone, o clipe MP4 e os WAVs da pasta `examples/media/stage16/` foram criados para este exemplo. São simulações gráficas e efeitos sintéticos, não filmagens, gravações de campo, amostras, alertas ou resultados de laboratório. O clipe demonstra a camada de vídeo, trim, velocidade e crop; não representa uma estação real. O áudio usa voz pt-BR medida, cama procedural do app, ambiência e efeitos sonoros sintéticos com ducking.

## Precisão editorial

O CDC descreve coagulação, floculação, sedimentação, filtração e desinfecção como etapas comuns, mas o processo necessário depende da água de origem e do sistema. O Ministério da Saúde descreve o Vigiagua, o monitoramento e o padrão brasileiro com mais de cem parâmetros; o vídeo não simula uma amostra nem afirma que um município específico cumpre os parâmetros.

- [CDC — How Water Treatment Works](https://www.cdc.gov/drinking-water/about/how-water-treatment-works.html)
- [Ministério da Saúde — Vigiagua](https://www.gov.br/saude/pt-br/composicao/svsa/saude-ambiental/vigiagua/vigiagua/)
- [Ministério da Saúde — Monitoramento da qualidade da água](https://www.gov.br/saude/pt-br/composicao/svsa/saude-ambiental/vigiagua/monitoramento-da-qualidade-da-agua)
- [OMS — Guidelines for Drinking-water Quality](https://www.who.int/publications/i/item/9789240121225)

## Validação local

O projeto, a voz, o MP4, o storyboard, os quadros, a análise de qualidade e o apêndice técnico dos 72 nativos são produzidos em `artifacts/antes-do-primeiro-gole-stage16/`, ignorado pelo Git. O script `scripts/stage16_build_demo.py` recria a mídia original e as duas receitas. O apêndice instancia todos os 72 IDs oficiais em 12 páginas e compara o catálogo com os `nativeId` efetivamente gravados no projeto.

O relatório do exemplo verificou todos os 72 IDs e não encontrou cortes, colisões ou beats prematuros. Restaram dois avisos informativos de redução tipográfica moderada. Confira a sequência exportada; o analisador é uma triagem geométrica e não julga a qualidade artística.
