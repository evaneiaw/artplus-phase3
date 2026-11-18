# Template de spec F-###

```markdown
---
id: F-025
modulo: DocumentMarks.dll
ui: Marks > Create > Trim Marks
versao_ref: 25.0
status: spec
confianca: alta
evidencia: [GHIDRA-DocumentMarks-0x18001A2C0, CAP-20261118-03]
adr: ADR-012
tests: []
---

# F-025 — Trim Marks

## 1. Resumo

Cria quatro marcas de corte nos cantos da página, posicionadas fora da TrimBox.

## 2. Entrada

- Documento com ao menos uma página.
- TrimBox definida; seleção irrelevante.
- Pré-condição: unidade do documento em mm ou pt.

## 3. UI

Caminho: `Marks > Create > Trim Marks`.

| Campo | Tipo | Default | Unidade | Faixa |
|---|---|---|---|---|
| Offset | numérico | 3 | mm | 0–20 |
| Length | numérico | 5 | mm | 1–50 |
| All sides | booleano | true | — | — |

Atalho: nenhum observado.

## 4. Regras

1. Para cada canto da TrimBox, criar uma marca em L.
2. A marca começa a `offsetMm` da borda e se estende por `lengthMm`.
3. Espessura fixa de 0,25 pt, cor registration.
4. `allSides=false` cria apenas nos cantos superior-esquerdo e inferior-direito.
5. A marca não pode invadir a TrimBox.

## 5. Exceções

| Condição | Resultado |
|---|---|
| offset < 0 | erro "Offset must be positive"; nada é criado |
| página sem TrimBox | usa CropBox e registra aviso |
| documento vazio | erro explícito, sem crash |

## 6. Saída

Quatro objetos `PathObject` na camada `Marks`; gravados no PDF como traço 0,25 pt na separação Registration.

## 7. Rastreabilidade

| Evidência | O que prova | Caminho |
|---|---|---|
| GHIDRA-DocumentMarks-0x18001A2C0 | cálculo da geometria | `rag/evidence/F-025/ghidra/...` |
| CAP-20261118-03 | campos e resultado visual | `rag/evidence/F-025/behavior/...` |

## 8. Em aberto

- A marca usa miter ou butt cap? (pergunta ao `comportamento`, zoom máximo)
```
