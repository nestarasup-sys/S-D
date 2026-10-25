# ADR 0010: intensidade da edição como defaults de movimento

**Status:** aceito e implementado parcialmente na receita v3.

## Contexto

A receita precisa aceitar uma intenção global de ritmo sem trocar as decisões explícitas que a IA já escreveu. Aplicar cortes ou efeitos em intervalos fixos ignoraria o significado narrativo e poderia desfazer uma direção deliberada.

## Decisão

- A receita v3 aceita `editing.intensity`, inteiro de 0 a 100.
- O parâmetro escolhe defaults para `motion`, `motionFrames` e `exitFrames` de cada elemento, somente quando a propriedade correspondente foi omitida.
- Intensidade 0 desliga o movimento e a saída automáticos. 1–30 usa fade mais lento; 31–70 usa rise; 71–100 usa pop progressivamente mais rápido.
- Se `editing` estiver ausente, a receita mantém exatamente os defaults anteriores. Receitas v1/v2 não aceitam a propriedade.
- Valores explicitamente definidos pela IA prevalecem. O parâmetro não cria cortes, beats ou efeitos por conta própria.

## Consequências

- A IA pode comunicar um perfil de energia global sem repetir timings em todos os elementos.
- O resultado é resolvido durante a importação e vira propriedades comuns de cada nó, mantendo a prévia e exportação sincronizadas.
- Ainda falta guardar/editar o perfil global no projeto aberto. A intensidade atual não gera decisões narrativas, transições ou mudanças de composição automaticamente.

## Verificação

- Testes de receita verificam limites 0–100, materialização determinística de defaults e preservação de movimentos explicitamente declarados.
