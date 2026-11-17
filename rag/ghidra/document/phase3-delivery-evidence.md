# Fase 3 — Evidência de entrega (atualizada 2026-11-17)

## Status

- Build Release 100% limpo, 5/5 CTest passando.
- Importador PDF validado em fixtures que exercitam textos, vetores, imagens, clipping, CMYK/RGB/Gray/Spot, múltiplas páginas, Form XObjects e anotações.

## Confirmação via CTest (-V)

```
test 1: FormXObjects .....................   Passed    0.16 sec
test 2: PdfSmoke ..........................   Passed    0.17 sec
test 3: UnitTests ........................   Passed    0.16 sec
test 4: PdfRoundTrip .....................   Passed    0.21 sec
test 5: EditingTests .....................   Passed    0.19 sec
100% tests passed out of 5
```

## Form XObjects (`/Subtype /Form`)

- Fixture: `tests/fixtures/test_formxobject.pdf` (reportlab `beginForm`/`doForm` invocado duas vezes).
- Como o importer trata: vê `/Do` → resolve o XObject pelo key → checa `DictGet("Subtype")` == "Form" → cria `GroupObject` com `setTransform(ctx.ctm)` → parse recursivo do stream do form com `initialCtm=identity` e `pageHeight` derivado de `/BBox` → filhos (paths/texts) adicionados a uma `childLayer` e ao `groupObj` via `addChild(id)` — preservando hierarchy.
- Evidência: grupo(s) presentes, 2 paths, 3 texts no fixture.
- Limitação: form matrix não é combinada corretamente para orientação Y invertida (pode renderizar invertido quando o form usa `Matrix` não-identidade); a composição de transform precisa de mais trabalho.

## Anotações

- Fixture: `tests/fixtures/test_unsupported.pdf` com `linkURL("https://example.com", ...)`.
- Como o importer trata: no loop por página, se `page.dictGet("Annots")` é array, emite warning "annotation(s) — not yet supported".
- Teste: `TestFormXObjects::testUnsupportedElementsProduceWarnings` verifica que um warning contendo "annotation" aparece no `res.warnings`.

## Endereços relevantes (Fase 2, Document.dll)

| Área | Address | Uso |
|---|---|---|
| `ReadOptions` ctor | `1800161e0` | Objetivo: replicar semântica de split de páginas, mas implementação atual simplificada |
| `PageRanges` | `1800156d0` | Usamos traversal `Catalog→Pages→Kids` |
| `ContentObject` | `180015000` | Nome de abstração referenciado no nosso `DocumentObject` (Ghidra evidence); não impacta implementação |
| `Representation` | `180016380` | Base para RGB/CMYK/Lab; implementamos como `ArtPlus::Color::Type::{CMYK,RGB,Lab,Gray,Spot}` |

## Limitations (reconfirmadas)

- Form matrices não identidade podem girar/espelhar children incorretamente (precisa de composição mais rigorosa de transforms).
- `SMask`/Transparency group não são reproduzidos no export regenerado (lossy: só `DeviceRGB` com `FlateDecode`).
- Sem undo/redo.
- Sem real interpretação de Form dictionaries (`Resources` aninhados do form ignorados pelo content parser).
- `Annots` não são parseados nem representados no modelo.

---
*Fontes: `rag/ghidra/exports/Document.dll/functions.csv`, `strings.csv`, `symbols.csv`; tests fixtures gerados por ReportLab/PIL; validação via PyMuPDF e execução direta do ArtPlus.*
