# ADR 0030 — cor independente para palavra destacada

Status: Aceito

## Contexto

O realce sincronizado de palavras usava sempre `primaryColor`, impedindo que a identidade visual da cena e o destaque de leitura fossem ajustados independentemente.

## Decisão

- `ArtDirection.captionHighlightColor` configura o realce globalmente, por cena ou por shot e pode ser informado em `artDirection` de uma receita v3.
- Se o campo não for informado, mantém-se o comportamento anterior: o realce acompanha `primaryColor`.
- Projetos persistem o campo no schema 14. Projetos schema 1–13 continuam aceitos e recebem o fallback anterior ao carregar.
- Prévia e exportação usam a mesma cor; o inspetor expõe sua edição.

## Consequências

Produções antigas preservam sua aparência. Receitas e projetos que personalizam o realce registram o campo explicitamente e podem manter a cor mesmo quando a cor primária muda.
