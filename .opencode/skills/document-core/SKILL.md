---
name: Document Core
description: Mantém o modelo central do ArtPlus — Document, Page, Layer, objetos, caixas de página, cores, seleção, undo e serialização. Use ao adicionar ou alterar tipo de objeto, camada, página, comando de undo ou formato de documento.
metadata:
  version: "1"
---

# Modelo central do documento

Fonte: `src/document/document_model.h`, `undo.h`, `document_serializer.*`.

## Estrutura

| Tipo | Papel |
|---|---|
| `Document` | dono de páginas, camadas, objetos, seleção e `filePath`; flag `modified` |
| `Page` | `PageBoxes` (media/crop/bleed/trim/art) + ordem de camadas |
| `Layer` | visibilidade, lock, printable, cor de UI, opacidade, ordem de objetos |
| `DocumentObject` | base abstrata: id, nome, visível, locked, `QTransform`, `FillStroke`, `layerId`, `parentId`, `softMaskId`, `clipMaskIds`, filhos |
| `PathObject` | `PathElement` Move/Line/Curve/Close; gradiente de fill/stroke |
| `TextObject` | texto, família, tamanho, bold, italic |
| `ImageObject` | `QImage` embutida |
| `GroupObject` | agrupamento |
| `ClippingMaskObject` | clipping path (W/W*), `evenOdd` |
| `OpacityMaskObject` | SMask, imagem + modo Alpha/Luminosity |

IDs: `ObjectId`, `LayerId`, `PageId` = `uint32_t`. IDs são estáveis durante a vida do documento; não renumere ao salvar.

## Unidades

- Coordenadas do modelo em **ponto PDF (1/72")**; `PageBoxes` default 595×842 pt (A4).
- A UI e a spec usam `mm`/`pt` explícitos; converta na borda (`pt = mm * 72 / 25.4`).
- Não guarde `double` sem unidade em campo novo. Nomeie `offsetMm`, `widthPt`, `resolutionDpi`.

## Invariantes

1. Todo objeto está em exatamente uma camada (`layerId`) e pode estar em uma hierarquia (`parentId`/filhos).
2. `addObject`/`removeObject` atualizam o mapa e a ordem da camada na mesma operação.
3. Remover página/camada remove ou realoca os objetos de forma atômica; não deixe ID órfão.
4. Seleção é estado de UI no documento; não altera geometria.
5. `modified` só muda em operação que altera conteúdo persistível.
6. Clone é profundo para conteúdo e preserva ID apenas quando a operação exigir (ex.: undo).
7. Soft mask e clip mask referenciam objetos existentes; ao remover o alvo, limpe a referência.

## Undo

`Command` tem `undo(Document&)`, `redo(Document&)` e `description()`. Comandos existentes: `TransformCommand` e `DeleteObjectsCommand`.

- Toda edição que muda conteúdo entra em um comando; agrupe operações de um mesmo gesto.
- `redo` deve reproduzir exatamente o estado anterior ao `undo`.
- Comando não pode capturar ponteiro solto para objeto que pode ser removido; guarde IDs e snapshots.
- Undo de seleção pura não é necessário; undo de transformação, inserção, exclusão e mudança de estilo é.

## Serialização

`document_serializer.cpp` define o formato. Regras:

- Versione o formato no próprio arquivo/JSON (`format_version`).
- Campo novo tem default compatível com arquivo antigo.
- Não serialize ponteiro, `QImage` cru sem codec, nem estado derivado.
- Teste roundtrip: salvar → abrir → comparar estrutura e geometria com tolerância.

## Testes mínimos ao mudar o modelo

- `tests/unit/test_editing.cpp` — edição e undo.
- `tests/unit/test_pdf_roundtrip.cpp` — import/export.
- Teste novo cita `F-###` no topo.
- Casos de borda: página sem camada, objeto sem pai, máscara órfã, ID duplicado, documento vazio.

## Limites

- Não mude assinatura pública de `Document`/`DocumentObject` sem ADR.
- Não coloque regra de prepress no modelo; motores ficam em `src/prepress/`.
- Não introduza dependência de Qt Widgets no modelo.

## Referências

- `references/model-invariants.md` — invariantes detalhados, fluxo de ownership e checklist de alteração.
