---
name: Screening and Halftone
description: Modela retículas e meio-tom no ArtPlus — lineatura, ângulos, formato de ponto, AM/FM, roseta e influência na saída. Use ao especificar screening, comparar com o ArtPro+ ou avaliar se o dado deve entrar no PDF.
metadata:
  version: "2"
---

# Retículas e meio-tom

Estado atual: **não há módulo de screening em `src/`**. Não implemente por analogia; exige spec e evidência.

## Conceitos

| Termo | Significado |
|---|---|
| LPI/LPC | linhas por polegada / centímetro |
| AM | retícula amplitude-modulada (ponto varia de tamanho) |
| FM | frequência modulada (ponto varia de densidade) |
| Ângulo | rotação da retícula; conjunto forma a roseta |
| Formato | redondo, elíptico, losango, estocástico |
| Roseta | padrão de interferência entre canais |

## Regras

1. Unidade explícita: `lpi` ou `lpc`; ângulo em graus; resolução de saída em `dpi`.
2. Screening de saída pode ser responsabilidade do RIP, não do PDF. Declare **onde** o dado vive.
3. Não invente ângulos “padrão” (ex.: 45°) sem evidência do original.
4. Separação e screening são camadas diferentes: uma define tinta, outra define como ela é impressa.
5. Mudar screening não altera geometria nem cor nominal do documento.
6. Se o ArtPlus só exibe preview, o dado não deve ser gravado no PDF como se fosse definitivo.
7. Amostra de roseta é derivada; recalcule a partir dos parâmetros, não guarde bitmap.

## Evidência

Captura do diálogo de screening/output do ArtPro+, valores aceitos e preview; PDF exportado para verificar se o dado aparece. Sem isso, a spec fica `em aberto`.

## Limites

- Não crie `src/prepress/screening.*` sem ADR.
- Não afirme compatibilidade com RIP sem teste real.
