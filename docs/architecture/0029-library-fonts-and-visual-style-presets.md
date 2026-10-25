# ADR 0029 — fontes externas e presets visuais nomeados

Status: Aceito

## Contexto

A direção de arte já era configurável globalmente, por cena e por shot, mas dependia de fontes instaladas no computador e precisava ser reconstruída manualmente em cada receita. O último item pendente da Fase 3 pede fontes externas, presets visuais reutilizáveis e documentação das capacidades reais para a IA.

## Decisão

- A biblioteca copia fontes TTF/OTF validadas para `library/fonts`, registra a família com Qt no aplicativo e no processo de renderização e mantém o item reversível no catálogo.
- Um preset visual guarda um objeto `ArtDirection` completo como ativo `visual-style` da biblioteca.
- A receita v3 aceita `artDirectionPreset` na raiz, cena e shot. A ordem é preset global, preset de cena, preset de shot e, por último, campos `artDirection` inline no mesmo escopo.
- Os IDs e arquivos permanecem locais à instalação; o guia gerado inclui os IDs disponíveis e a família de cada fonte.
- O editor permite pré-visualizar as cores na biblioteca, escolher fonte, salvar estilo e aplicar ao escopo selecionado como uma edição única.

## Consequências

A prévia e o render carregam fontes importadas do catálogo local. Transferir uma produção requer importar a mesma fonte e obter os IDs de estilo da biblioteca de destino. Elementos com cor ou tamanho explícito mantêm prioridade sobre os defaults da direção de arte.
