# Fase 3 — Evidência de entrega (atualizada 2026-11-17)

## Estado final verificado

- Build Release 100% limpa; **5/5 testes CTest verdes**.
- Importador PDF com **sequência ordenada de eventos** (Path/Text/Image/Clip/Push/Pop); artefatos preservados como objetos de primeira classe.
- **ClippingPath** (`W`/`W*`) preservado como `ClippingMaskObject` no modelo; **render** (QPainter com setClipPath) e **exportador** (operador W/W* antes do paint) implementados; round-trip save/load com remapeamento de IDs.
- **Alpha real** (`/ca` e `/CA`) é lido do `ExtGState` e aplicado: (a) no canvas via opacidade do item; (b) no PDF exportado via `/GS` ExtGState dedicado.
- **Imagens** exportadas no PDF regenerado como Image XObjects (`DeviceRGB` + FlateDecode lossless), com asserção automatizada em `testExportEmbedsImageXObjects`.
- **Anotações** (`/Annots`): warning explícito, nunca silenciosamente descartadas.
- **Form XObjects** (`/Form`): reparseados em grupo; filhos adicionados como objetos e agrupados via `children()` + `GroupObject`.

## CTest (-V direto)

```
1/5 FormXObjects .............. Passed
2/5 PdfSmoke .................. Passed
3/5 UnitTests ................. Passed
4/5 PdfRoundTrip .............. Passed
5/5 EditingTests .............. Passed
100% tests passed out of 5
```

## Endereços Ghidra relevantes (Fase 2, Document.dll)

| Área | Address | Uso nesta fase |
|---|---|---|
| `ReadOptions` ctor | `1800161e0` | Preservação de metadados/opções na abertura |
| `PageRanges` ctor | `1800156d0` | Travessia `Catalog→Pages→Kids` |
| `ContentObject` | `180015000` | Abstração equivalente ao `DocumentObject` |
| `Representation` | `180016380` | RGB/CMYK/Lab/Gray no modelo |
| `ColorConverter` | `1800271c0` | CMYK/Spot preservados na importação; export DeviceRGB |
| `Transparency` | `18001ca50` | Blend alpha lido/emitido via ExtGState; soft masks não preservados |

## Validação visual (screenshots, 2026-11-17)

- `page1_text2.png` — Textos reais renderizados (Helvetica-Bold vermelho, Times-Roman, Courier-Bold) com posicionamento correto
- `page2_final.png` — **Clipping exibido**: retângulo laranja recortado pelo círculo de clipping; texto da página visível
- `page3_final.png` — Transparência real: dois retângulos sobrepostos com `/ca` = 0.5
- `images_fixed2.png` — Raster embutido desenhado no quadrado unitário
- `artplus_clip_demo.png` / `page2_demo.png` — mesma evidência do clipping em execução

## Limitações reconfirmadas

- Form matrices não-identidade podem compor transforms incorretamente (children de `Form /Do`).
- `SMask` preservado apenas como opacidade plana (perda de soft masks).
- Sem undo/redo (infra `ActionList` não replicada).
- PDF regenerado re-embute imagens em RGB sem perfis CMYK.
- Fonts exportadas usam `BaseFont /Helvetica` apenas (sem subsetting/hex).

---
*Fontes: `rag/ghidra/exports/Document.dll/functions.csv`, `strings.csv`, `symbols.csv`; fixtures ReportLab/PIL; PyMuPDF para validação; screenshots via Win32 PrintWindow/SetForegroundWindow.*
