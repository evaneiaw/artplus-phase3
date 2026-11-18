---
name: Prepress Trapping
description: Aplica trapping no ArtPlus — spread, choke, zona de trap, regras preto/branco, trap manual e trap em step & repeat. Use ao implementar TrappingEngine, definir parâmetros ou validar trapas contra o original.
metadata:
  version: "2"
---

# Trapping

Código: `src/prepress/trapping.h/.cpp` (`TrappingEngine`).
Tipos: `Settings`, `TrapZone`, `Result`, `Analysis`, `BlackWhiteRules`.

## Conceitos

- **Spread**: a tinta clara invade a escura. **Choke**: a escura invade a clara.
- **Trap width**: largura em `mm`, derivada de registro e lineatura; não é constante universal.
- **Zona**: par de tintas adjacentes onde o trap se aplica.
- **Preto**: regra própria (preto sobre preto, preto sobre cor, cor sobre preto).

## Regras

1. Unidade em `mm`; sinal define spread/choke, não texto ambíguo.
2. Trap só entre tintas adjacentes com contraste suficiente; declarar limiar.
3. Não aplicar trap em objeto isolado ou sobre fundo transparente sem regra.
4. **Preto 100%** segue `BlackWhiteRules`; não trate como mais uma cor.
5. Trap manual é dado do usuário: persiste, tem ID e pode ser removido; não é recalculado.
6. Trap em step & repeat replica a geometria por cópia, sem acumular espessura.
7. Resultado é determinístico e não altera o documento fonte até `apply` explícito.
8. Registrar quantidade de traps, pares de tinta e zona; sem isso não há auditoria.

## Evidência

Captura do diálogo de trapping (defaults, faixa), PDF com overprint/trap e inspeção de um caso spread e um choke. Sem isso, não invente largura.

## Testes

- Dois retângulos CMYK adjacentes → trap na borda correta, largura esperada.
- Preto sobre ciano → regra de preto aplicada.
- Sem adjacência → zero traps (não falso positivo).
- Undo remove todo o trap aplicado.

## Limites

- Não altere `src/document/` para representar trap; proponha ADR.
- Não copie fórmula de trapping de fonte não citada.
