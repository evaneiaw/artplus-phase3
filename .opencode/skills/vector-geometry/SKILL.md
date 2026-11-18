---
name: Vector Geometry
description: Implementa geometria vetorial no ArtPlus — Bézier, paths, booleanos, offset, simplificação, junção, corte e máscaras com tolerância numérica. Use ao editar traçado, calcular contorno, unir/subtrair formas ou corrigir precisão.
metadata:
  version: "2"
---

# Geometria vetorial

Código: `src/geometry/geometry_engine.h`, `path_operations.h`.

## Modelo

- `Geometry::Point` (float) e conversão para `QPointF`.
- `BezierCurve`: `at(t)`, `tangent(t)`, `split(t)`.
- `PathOperations`: `boolean` (Union/Intersection/Difference/Xor), `offset`, `simplify`, `smooth`, `joinPaths`, `splitPath`, `retracePath`, `adjustCorners`, `cutPath`, `spread`, `choke`, `douglasPeucker`.
- Documento usa `PathObject` com `PathElement` Move/Line/Curve/Close.

## Regras

1. Toda operação declara **tolerância** (ex.: 0,01 mm); sem isso o resultado é irreprodutível.
2. Path fechado vs aberto é explícito; offset de path aberto tem regra própria.
3. Regra de preenchimento: `nonzero` ou `even-odd`; não troque por conveniência.
4. `float` é suficiente para tela, **não** para acumular milhares de operações: use `double` em cálculo intermediário quando a precisão importar.
5. Booleanos podem falhar em path autointersectado; detecte e devolva erro, não geometria corrompida.
6. Offset negativo = choke; positivo = spread. Sinal documentado.
7. Simplificação não pode mover vértice além da tolerância declarada.
8. Bézier com pontos coincidentes ou tangente zero é caso de borda testado.
9. Resultado não pode ter `NaN`/`inf`; valide antes de gravar no documento.
10. Undo restaura os elementos originais, não o path recalculado.

## Evidência

Comportamento do ArtPro+ para a operação (resultado visual + PDF) e a spec com tolerância. Sem isso, não invente algoritmo.

## Testes

- União/intersecção de dois retângulos → área esperada.
- Offset +2 mm e −2 mm → distância do contorno.
- Simplificar curva com epsilon 0,1 mm → desvio máximo ≤ 0,1 mm.
- Path autointersectado → erro explícito.
- `split(t)` → curvas que reconstroem a original dentro da tolerância.

## Limites

- Não troque a engine por biblioteca externa sem ADR.
- Não use `QPainterPath` como armazenamento definitivo sem conversão explícita.
