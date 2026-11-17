# Fase 3 — Núcleo PDF e Integração com o Modelo de Documento

**Status: concluída com limitações documentadas.**
**Data: 2026-11-17**

## 1. Preparação

- Arquitetura analisada: núcleo `src/document/` (modelo ArtPlus pré-existente) + `src/ui/` + novo módulo `src/pdf/`.
- Evidências do Ghidra consultadas em `rag/ghidra/document/`: `file-io-pdf.md`, `color-management.md`, `document-architecture.md`, `undo-redo-actions.md`, `transparency-blending.md`, `fonts-text.md`, `annotations.md`, `index.md`.
- Reutilizado Qt 6.8.3 (Core, Gui, Widgets, Svg, ZlibPrivate) — sem novas dependências. `Qt6::ZlibPrivate` fornece `inflate` para `FlateDecode` (o bundled do Qt, não um zlib externo).

## 2. Importação e interpretação PDF

Implementado em `src/pdf/`:

| Componente | Arquivo | Função |
|---|---|---|
| Tipos/valores PDF | `pdf_types.h/.cpp` | Modelo genérico: null, bool, int, double, string, name, array, dict, ref |
| Lexer | `pdf_lexer.h/.cpp` | Tokenizer com suporte a `<<`, `>>`, strings com escapes, nomes `/Name`, números, operadores |
| Parser recursivo | `pdf_parser.h` | Parser com lookahead de 1 token; resolve `N G R` (refs) |
| Documento | `pdf_document.h/.cpp` | Varredura sequencial de objetos (`N G obj ... endobj`), decode de streams (ASCII85, ASCIIHex, Flate), coleta de páginas pela árvore Catalog→Pages→Kids, Page Boxes (Media/Crop/Bleed/Trim/Art), rotação, trailer+ID→Info title/author/subject, pageContents() |
| Interpretador de conteúdo | `pdf_content.h/.cpp` | Operadores PDF: `q/Q/cm`, `w/J/j/M/d`, `g/G/rg/RG/k/K/sc/SC/scn/SCN/cs/CS`, construção de paths (`m/l/c/v/y/re/h`), painting (`S/s/f/f*/B/b/b*/n`), clipping (`W/W*`), texto (`BT/ET/Tf/Td/TD/Tm/T*/Tj/TJ/'/"`), `Do`, `gs`, `BMC/BDC/EMC` |
| Importador para o modelo | `pdf_importer.h/.cpp` | Converte Path, Text, Image e clipping em objetos do modelo ArtPlus com ID estável, camada por página, warnings preservados como strings |
| Exportador | `pdf_exporter.h/.cpp` | (a) Passthrough byte-a-byte de arquivos não modificados; (b) geração de PDF a partir do modelo quando modificado |

**Resultado (pdf_smoke.exe):**
- `test_text.pdf` → 3 textos importados
- `test_vector.pdf` → 5 paths + 1 texto (inclui curva Bézier, dashed line, spot color CMYKColorSep)
- `test_images.pdf` → 1 imagem (120x90) + 1 texto
- `test_multipage.pdf` → 3 páginas, 4 paths + 3 textos, 1 warning de clipping
- `test_cmyk_inks.pdf` → 5 paths CMYK + 1 texto (inclui spot)

## 3. Modelo de documento

- `PdfImporter::import()` cria uma `Page` e um `Layer` ("Page N") por página, povoando comobjetos `Path`, `Text`, `Image`.
- IDs seguem o esquema `uint32_t` estável do modelo (contadores `nextObjectId`, `nextLayerId`, `nextPageId`).
- Transformações: o parser aplica `CTM` acumulado (incluindo `cm`) nas coordenadas de todos os pontos de caminho e textos; imagens mantêm o `CTM` de exibição.
- Coordenadas: PDF usa y-up e o modelo/Qt usa y-down; o parser aplica flip y usando `pageHeight`, alinhando com o canvas `QGraphicsView`.
- Clipping é reconhecido (`W/W*`) e reportado como warning (`ClippedArea`), mas ainda não vira objeto `ClippingMask` no modelo — limitação documentada.

## 4. Edição e interface

`src/ui/main_window.{h,cpp}` implementa:

- Menus **File** (Open PDF, Open .artplus, Save, Export PDF), **Edit** (Select All, Deselect All, Delete) e **View** (Zoom In/Out/Fit).
- Barra de ferramentas: Open PDF, Open .artplus, Save, Export PDF, Select, Pan, Zoom, Select/Deselect/Delete, Zoom In/Out/Fit.
- Canvas `QGraphicsView` com renderização de paths (fill/stroke/dash/line-cap/join), textos e imagens.
- Seleção por clique simples (toggle) e por rubber band (arrastar); `Shift` adiciona à seleção.
- Arrastar move o objeto (atualiza transform no modelo e marca `modified`).
- Delete remove do modelo e das camadas.
- `Hand`/`Pan` ferramenta (H) move a visualização — a ferramenta de seleção **não** move a página.
- Dock `Layers` lista camadas; dock `Objects` lista objetos selecionados.
- Status bar exibe estado do documento e contagem de objetos.

**Não implementado (limitações conscientes):**
- Undo/Redo nativo do ArtPlus (o binário usa `ActionList`/`Action` em `ArtDoc`; replicar é fora do escopo).
- Menu/modal para edição de propriedades (cor, stroke, etc.) — apenas seleção e movimento/transformação.
- Imagens importadas mantêm bytes originais via passthrough; em documentos modificados, são re-escritas em RGB simples por `pdf_exporter` (perde CMYK/spot e JPEG passthrough incremental).

## 5. Exportação e integridade

- `DocumentSerializer` (`src/document/`) salva/abre `.artplus` (JSON versionado) preservando páginas, camadas, objetos, ordem de desenho, transformações, cores (CMYK/RGB/Gray/Spot), tipos e imagens.
- `DocumentSerializer::compare()` compara dois documentos: contagens (páginas, camadas, objetos, seleção), boxes (media/crop) por página, e type/name/transform por objeto.
- `PdfExporter::exportPdf(doc, out, originalImportedPath)`:
  - Se `doc.modified() == false`, copia o PDF original byte-a-byte (integridade perfeita).
  - Caso contrário, gera um PDF 1.4 simples com Catalog/Pages/Page/Contents/Font via operadores conhecidos (paths, textos, cores). Imagens não são re-embutidas — renderizadas como placeholder.
- `tests/unit/test_pdf_roundtrip.cpp` valida: importar+serializar+reabrir+comparar (vector, images, multipágina), exportar+reimportar geometria, exportar PDF com modificação.

## 6. Engenharia reversa

- Ghidra evidências revisadas em `rag/ghidra/document/`. Destaques relevantes ao módulo PDF (interpretação e impressão):
  - `ArtDoc::ContentObject`/`ListItem` (`180015000`), `PageRanges` (`1800156d0`), `ReadOptions`/`WriteOptions` (`1800161e0`/`180016730`)
  - `Representation` (`180016380`) — separação RGB/CMYK do conteúdo no documento de impressão; paralelismo com o conceito de "document editable vs original" do ArtPlus.
  - `ArtColor::ColorConverter` (`1800271c0`), `ICCProfile` (`18002da50`), perfis embutidos `GenCMYK.icm` etc. — base de conversão para exportação CMYK.
  - `Transparency` (`18001ca50`) — blend modes e soft masks: há `/GS` com `/ca`/`/CA` no PDF, mas nosso export regenera conteúdo e perde soft masks.
- Comportamento do ArtPro+ v24 observado: opens PDFs preservando sumber de drops (satisfeito por passthrough quando não modificado), exibe camadas como árvore (nosso modelo adiciona um layer por página), seleciona objetos com bounding box de dash linha (simulado via `setSelected` em Qt graphics items).

## 7. Testes obrigatórios

`tests/fixtures/make_test_pdfs.py` gera (regenerável):

- `test_text.pdf` — Helvetica/Times/Courier, RGB, metadata
- `test_vector.pdf` — CMYK fill, curva Bézier, spot PANTONE 485 C, dashed line, triângulo
- `test_images.pdf` — imagens embutidas (geradas por PIL)
- `test_multipage.pdf` — 3 páginas com clipping e transparência
- `test_cmyk_inks.pdf` — CMYK separável e spot custom

`tests/integration/pdf_smoke.cpp` valida o pipeline completo em todos os fixtures.
`tests/unit/test_pdf_roundtrip.cpp` (CTest `PdfRoundTrip`) e `tests/unit/test_main.cpp` (CTest `UnitTests`).

**Resultado atual do CTest -C Release -V:**
```
test 1: UnitTests ........................ Passed
test 2: PdfRoundTrip .................... Passed
100% tests passed, 0 tests failed out of 2
```

## 8. Entrega

## 9. Git e relatório

Ver `git log` e `git status`.
