# ADR 0005 — receita v2 e manifesto local da biblioteca

Data: 24/10/2026. Estado: adotada.

## Contexto

O contrato de produção v1 exigia cabeçalho, kicker e fundo por cena e só aceitava elementos planos. O projeto evoluiu para schema 3, mas a IA ainda não podia pedir grupos, keyframes, câmera ou objetos salvos na biblioteca pessoal. O botão de exportar guia também não incluía o catálogo ativo.

## Decisão

- Aceitar `studio-dark-production` v2 mantendo o leitor v1 compatível.
- Manter narração em trechos locais à cena; cues e offsets em segundos são resolvidos a partir do WAV sintetizado real.
- Permitir composição sem título, kicker ou cenário automático; esses elementos viram escolhas explícitas.
- Representar grupos com `kind: "group"` e filhos recursivos até oito níveis. Limitar produção inteira a 200 elementos.
- Resolver `keyframes` de transformação e `camera` a partir de cues; usar interpolação linear e os mesmos frames em prévia/exportação.
- `captions: false` desliga apenas legendas visuais; SRT permanece disponível.
- Resolver `kind: "library", assetId` apenas contra itens ativos do catálogo local. Mídias mantêm paths locais; não se faz fetch nem download remoto. Itens removidos e caminhos ausentes causam erro.
- O guia salvo no editor lista itens pessoais ativos e categorias naquele momento. O guia do repositório, gerado sem acesso ao perfil local, instrui a pessoa a salvar sua cópia pelo aplicativo.
- Para manter o pacote atual seguro e compreensível, a receita não pode executar código, resolver URLs, nem referenciar itens apagados. Bibliotecas locais não são transferidas junto com o `.production.json`.

## Limites atuais

A biblioteca agora salva grupos completos com filhos e keyframes relativos, com cópias de imagem/SVG. A instância editável remapeia todos os IDs e permite desfazer a árvore em uma operação. `kind: "library"` em receita v2 expande presets compostos, preserva animações e verifica se os keyframes cabem no intervalo de áudio medido. Persistem limites de seleção múltipla, edição de instâncias vinculadas e empacotamento portátil. IDs são específicos da instalação; se a cópia local do catálogo muda, a importação informa que o item não existe. A receita ainda não tem eventos por palavra, documentos com regiões, vídeo, estilo configurável ou atualização parcial de projeto.

## Validação de implementação

O build C++ do desktop e do worker de produção precisa concluir. Casos adicionais foram escritos para presets compostos e importação por JSON; a execução da suíte, o build e os smokes ainda precisam ser confirmados no Windows com Qt 6.8. O exemplo v2 é mantido em `examples/composicao-livre-v2.production.json`; o guia de produção é gerado do código em `modules/production/src/guide.cpp`.
