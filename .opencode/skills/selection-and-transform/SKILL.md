---
name: Selection and Transform
description: Implementa seleção, transformação e manipulação de objetos no ArtPlus — IDs, caixa de seleção, QTransform, snapping, undo e coordenadas. Use ao editar objetos na cena, criar ferramenta de seleção ou corrigir transformação.
metadata:
  version: "2"
---

# Seleção e transformação

Código: `Document` (`selectedObjects`, `selectObject`, `deselectObject`, `clearSelection`) e `undo.h` (`TransformCommand`).

## Regras

1. Seleção guarda **IDs**, não ponteiros; objeto removido sai da seleção.
2. Seleção múltipla preserva ordem de seleção quando a operação depender dela.
3. Transformação é `QTransform` aplicada na ordem declarada; documente se é local ou global.
4. Undo guarda transformação **antes e depois** por objeto (`TransformCommand`), não o delta acumulado.
5. Um gesto de arrastar = um comando de undo, mesmo com vários `mouseMove`.
6. Rotação e escala usam pivô explícito (centro da seleção, canto ou cursor) definido na spec.
7. Snap é cálculo de UI; não altera o modelo até o commit do gesto.
8. Objeto bloqueado ou camada bloqueada rejeita transformação com erro explícito.
9. Tolerância de hit-test em `mm`/px declarada; não use `==` em coordenada.
10. Coordenada do modelo em ponto PDF; conversão de tela só na borda.

## Evidência

Captura do ArtPro+ mostrando alça, pivô, valor exibido e resultado de mover/rotacionar/escalar. Sem isso, não invente default de snap nem incremento.

## Testes

- Mover 1 objeto → transformação e undo corretos.
- Mover 3 objetos → um undo restaura todos.
- Rotacionar 90° em pivô definido → bounding box esperado.
- Objeto bloqueado → erro, sem alteração.
- Seleção após remoção → ID ausente.

## Limites

- Não coloque hit-test no modelo de domínio.
- Não altere `TransformCommand` sem ADR.
