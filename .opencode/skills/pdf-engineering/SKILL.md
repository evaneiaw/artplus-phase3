---
name: PDF Engineering
description: Analisa, importa, edita e exporta PDF no ArtPlus — lexer/parser próprios em src/pdf, content streams, Form XObject, SMask, transparência, fontes e cores. Use ao depurar import/export, inspecionar PDF de referência, comparar roundtrip ou validar saída contra o ArtPro+.
metadata:
  version: "1"
---

# Engenharia de PDF

## Engine do projeto

| Arquivo | Papel |
|---|---|
| `src/pdf/pdf_types.*` | `PdfValue` (Null/Bool/Int/Double/String/Name/Array/Dictionary/Ref), `PdfObject`, `PdfError` |
| `src/pdf/pdf_lexer.*` | tokenização |
| `src/pdf/pdf_parser.h` | parsing de objetos e xref |
| `src/pdf/pdf_document.*` | documento, páginas, `PageGeometry` |
| `src/pdf/pdf_content.*` | content streams e operadores |
| `src/pdf/pdf_importer.*` | `PdfImporter::import(const PdfDocument&, Document&)` → `PdfImportResult` |
| `src/pdf/pdf_exporter.*` | `PdfExporter`: escape, cor, imagem, compressão Flate |

`PdfObject` guarda stream decodificado e `rawOffset` para round-trip. `PageGeometry` usa **1/72"**.
Não há CMM externo nem renderer externo no build: o desenho é Qt.

## Toolbox de análise (fora do produto)

Instalado e verificado no Python 3.12:

| Ferramenta | Uso |
|---|---|
| PyMuPDF (`fitz`) | abrir, listar objetos, renderizar página, extrair texto/imagem |
| pypdfium2 | render alternativo/independente |
| pdfminer.six | extração de texto e layout |
| PyPDF2 | metadados e merge simples |
| reportlab | gerar PDF de teste controlado |
| fonttools | inspecionar fonte embutida |
| Pillow | converter/inspecionar render |

Ausentes: `qpdf`, `mutool`, `gs`, `pdftoppm`. Não escreva receita que dependa deles.

## Receitas

Renderizar para comparar:

```powershell
python -c "import fitz; d=fitz.open(r'C:\caminho\a.pdf'); p=d[0]; p.get_pixmap(dpi=144).save(r'C:\temp\a.png'); print(d.page_count, p.rect)"
```

Inspecionar estrutura:

```powershell
python -c "import fitz; d=fitz.open(r'C:\caminho\a.pdf'); print(d.metadata); print(d.xref_length()); print(d.get_page_xobjects(0))"
```

Comparar dois PDFs por página (texto e tamanho):

```powershell
python -c "import fitz; a=fitz.open(r'A.pdf'); b=fitz.open(r'B.pdf'); print(a.page_count, b.page_count); [print(i, a[i].rect, b[i].rect, a[i].get_text()==b[i].get_text()) for i in range(min(a.page_count,b.page_count))]"
```

Para diferença visual, use a skill `visual-regression-testing`.

## Regras de implementação

- Content stream é bytes: não converta para `QString` sem preservar Latin-1/bytes.
- Form XObject tem matriz própria; composição é `parent * formMatrix`, não substituição.
- SMask é máscara de luminosidade/alfa; não descarte no import nem no export.
- Transparência: `/ExtGState` com `/ca`, `/CA`, `/BM`; preserve o blend mode suportado.
- Clipping: `W`/`W*` viram `ClippingMaskObject` com `evenOdd` correto.
- Cor: mantenha o espaço original (CMYK/Spot/RGB/Lab/Gray) até a borda de render.
- Fonte: mantenha nome, encoding e embedding; não substitua por Arial silenciosamente.
- Export: preserve objetos não editados quando possível; não reencode stream sem necessidade.
- Roundtrip: `rawOffset` existe para reemitir o que não foi alterado. Use-o.

## Fixtures e testes

`tests/fixtures/`: `test_vector.pdf`, `test_text.pdf`, `test_images.pdf`, `test_cmyk_inks.pdf`, `test_formxobject.pdf`, `test_form_transform.pdf`, `test_smask.pdf`, `test_multipage.pdf`, `test_unsupported.pdf`.

Alvos: `pdf_smoke`, `pdf_roundtrip_test`, `form_xobject_test`, `smask_probe`, `form_transform_probe`, `mirror_probe`, `page_placeholders`.

Todo teste novo cita `F-###`; tolerância geométrica explícita (ex.: 0,01 mm).

## Limites

- Não invente suporte a PDF 2.0, criptografia ou assinatura sem spec.
- Não use biblioteca externa no produto sem ADR e auditoria de licença.
- Não "conserte" o arquivo de referência para o teste passar.

## Referências

- `references/pdf-toolbox.md` — comandos de inspeção, render, extração e diagnóstico.
- `references/model-rules.md` — mapeamento PDF → modelo do ArtPlus e armadilhas.
