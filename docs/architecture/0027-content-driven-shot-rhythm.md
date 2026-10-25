# ADR 0027 — ritmo de cortes guiado pela narração

- Estado: Aceita
- Data: 2026-10-25

## Contexto

Receitas v3 permitiam que cada shot tivesse um início explícito, em quadros derivados da fala. Criar uma montagem inteira exigia escolher manualmente cada corte. Estimar o tempo pela quantidade de caracteres ou pela duração média das palavras poderia deslocar a edição da voz real.

## Decisão

- Manter o modo manual como padrão e acrescentar `editing.rhythm: "narration"` como escolha explícita da receita.
- No modo automático, todos os shots omitem `start`. Misturar inícios manuais e automáticos é inválido.
- Calcular candidatos só com timestamps reais de palavras da voz sintetizada. Pontuação, limites de fala e uma lista pequena de conectivos dão preferência a pontos de frase; a intensidade equilibra essa preferência com o espaçamento dos planos.
- Reservar um mínimo por shot em função da intensidade e da transição de entrada. Recusar o plano com mensagem específica se faltarem timestamps, candidatos ou duração suficiente.
- Ancorar no início calculado do shot elementos que omitem sua referência temporal, inclusive filhos de grupos. Preservar os horários explícitos; o projeto valida se cabem no shot.
- Não acrescentar campos ao schema do projeto: os cortes continuam sendo shots normais com início e duração em quadros.

## Consequências

- Uma receita pode distribuir shots por frases da fala depois que o sintetizador local fornece o alinhamento real.
- O resultado é determinístico e não depende de uma chamada a um serviço de IA durante a compilação. Os sinais usados são pontuação, limites de fala e conectivos; isso não é uma análise semântica livre.
- A geração depende de uma voz local que produza um timestamp por palavra. Se ela não fornecer alinhamento suficiente, o app não inventa tempos.
- Horários explícitos em elementos, keyframes e beats podem tornar uma escolha automática incompatível com o intervalo de um shot; a validação aponta o elemento/shot e os quadros envolvidos.
- A edição avançada de duração/overlap da sequência permanece uma capacidade separada e pendente.

## Verificação

- Testes cobrem receita automática, rejeição de mistura manual/automática, ausência de timestamps, escolha no limite de frase e herança do corte por elementos agrupados.
- `scripts/smoke-production.ps1` sintetiza a demonstração v3 com Microsoft Maria Desktop, confirma cobertura contígua dos três shots, valida beats, renderiza o MP4, compara quadros de preview/export e abre o projeto na interface.
