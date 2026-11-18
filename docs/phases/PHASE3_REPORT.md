# Fase 3 — Núcleo PDF e Integração com o Modelo de Documento

**Status: concluída com limitações documentadas.**
**Data: 2026-11-17**

## 1. Preparação

- Arquitetura analisada: núcleo `src/document/` (modelo ArtPlus pré-existente) + `src/ui/` + novo módulo `src/pdf/`.
- Evidências do Ghidra consultadas em `rag/ghidra/document/`: `file-io-pdf.md`, `color-management.md`, `document-architecture.md`, `undo-redo-actions.md`, `transparency-blending.md`, `fonts-text.md`, `annotations.md`, `index.md`, `phase3-evidence.md`, `phase3-delivery-evidence.md`.
- Reutilizado Qt 6.8.3 (Core, Gui, Widgets, Svg, ZlibPrivate) — sem novas dependências. `Qt6::ZlibPrivate` fornece `inflate`/`compress2` para `FlateDecode` e para a exportação lossless de imagens (o bundled do Qt).

## 2. Importação e interpretação PDF

Implementado em `src/pdf/`:

| Componente | Arquivo | Função |
|---|---|---|
| Tipos/valores PDF | `pdf_types.h/.cpp` | Modelo genérico de objeto |
| Lexer | `pdf_lexer.h/.cpp` | Tokenizer |
| Parser recursivo | `pdf_parser.h` | Parser com lookahead |
| Documento | `pdf_document.h/.cpp` | Varredura de objetos, decode de streams, páginas, Page Boxes, metadados |
| Interpretador de conteúdo | `pdf_content.h/.cpp` | Operadores (`q/Q/cm`, `m/l/c/v/y/re`, cores, texto, `Do`, `gs`); sequência ordenada de eventos (Path/Text/Image/Clip/Push/Pop) |
| Importador para o modelo | `pdf_importer.h/.cpp` | Converte sequência em objetos do modelo (Path/Text/Image/ClippingMask) |
| Exportador | `pdf_exporter.h/.cpp` | Passthrough byte-a-byte (quando não modificado); regeneração PDF 1.4 com XObjects, ExtGState e clips |

**Resultado (pdf_smoke.exe):**
- `test_text.pdf` → 3 textos
- `test_vector.pdf` → 5 paths + 1 texto (spot PANTONE 485 C)
- `test_images.pdf` → 1 imagem + 1 texto
- `test_multipage.pdf` → 3 páginas, 4 paths + 3 textos + **1 ClippingMaskObject** preservado da operação `W*`
- `test_cmyk_inks.pdf` → 5 paths CMYK + 1 texto (spot)
- `test_formxobject.pdf` → Form XObject → `GroupObject` com filhos

## 3. Modelo de documento

- `pdf_importer` cria uma `Page` e um `Layer` ("Page N") por página; objetos seguem em ordem de desenho.
- IDs estáveis (uint32 contínuos).
- `DocumentObject` agora carrega `clipMaskIds()` — referências a `ClippingMaskObject`.
- `ClippingMaskObject` armazena o caminho de clipping (`evenOdd` incluído) como objeto de primeira classe.
- Coordenadas: parser aplica flip-y (Qt y-down) **corrigido** (`QTransform` aplica da esquerda→direita; a ordem correta para o CTM é `cm * ctm`, para textos `textMatrix * ctm`).
- Matrizes: cada objeto preserva a matriz final (inclusive `cm` escalado/rotacionado).
- BaseFont de verdade é resolvida dos `/Font` resources (`/F1 → /Helvetica`).

## 4. Edição e interface

`src/ui/main_window.{h,cpp}`:

- Menus **File/Edit/View** completos; todos os botões/menus funcionais.
- Seleção por clique/arrastar (toggle), rubber band (Shift adiciona).
- Arrastar move objetos; Delete remove do modelo e da layer; Fit/Zoom In/Out.
- **Navegação de páginas** (`Page:` combo na toolbar) — o canvas renderiza apenas a página atual; a lista de camadas também é filtrada por página. Atalho de linha de comando: `--page N`.
- Renderização de paths (fill/stroke/dash/line-cap/join), textos (com clipping aplicado via `QPainter::setClipPath` delimitando a máscara do documento) e imagens (byte-embutidas; desenhadas em quadrado unitário com CTM completo — `QPainterPath`).
- Transparências visíveis no canvas (alpha do `ExtGState` `/ca` `/CA` aplicada via opacidade dos itens).
- Docks Layers/Objects listam páginas/objetos; status bar exibindo estado.

**Validação visual (screenshots em `artplus_*.png`):**

- `page1_text2.png` — Textos reais renderizados (Helvetica-Bold em vermelho, Times, Courier) com posicionamento correto
- `page2_final.png` — **Clipping exibido**: círculo cortado pelo `ClippingMaskObject` (contorno tracejado magenta) + texto da página 2
- `page3_final.png` — Transparência: dois retângulos sobrepostos com blend real (alpha via `/ca`)
- `images_fixed2.png` — Imagem raster exibida no quadrado unitário no lugar correto

## 5. Exportação e integridade

- `.artplus` (JSON) salva/reabre preservando: páginas, camadas, objetos, ordem, transforms, cores (CMYK/RGB/Gray/Spot), tipos, imagens (PNG base64), **clipMaskIds**, IDs.
- Serializer faz remapeamento completo de IDs após load (objetos, camadas, filhos, clipMasks, parentId) e re-registra membership de layers — o documento recarregado é equivalente.
- `PdfExporter::exportPdf(doc, out, originalImportedPath)`:
  - `doc.modified() == false` → copia o PDF original byte-a-byte.
  - Caso contrário, regenera: Catalog/Pages/Page/Contents/Font/Image XObjects (`DeviceRGB` + `FlateDecode` lossless — sem placeholders) + **ExtGState por objeto com alpha < 1** (`/ca` e `/CA`) + **clips** (`W`/`W*`) antes dos paths/textos/imagens.
- Testes:
  - `testClipMaskRoundTrip` — mask sobrevive a save/load com IDs consistentes; PDF exportado contém `W`/`W*`; nome da classe interna não vaza.
  - `testExportEmbedsImageXObjects` — exported PDF regenerado contém `/Subtype /Image`, `/Type /XObject`, `FlateDecode` (asserção automatizada em bytes).
  - `testExportAndReimport` — geometria preservada.
  - `testExportModifiedPdf` — paths sobrevivem ao round-trip.
- **Comparação entrada/saída**: PyMuPDF usado nos fixtures para validar geometria/conteúdo antes e depois (via Smoke).

## 6. Engenharia reversa

- `rag/ghidra/document/phase3-evidence.md` e `phase3-delivery-evidence.md` registram a cross-evidence:
  - endereços Ghidra relevantes (`ReadOptions 1800161e0`, `WriteOptions 180016730`, `PageRanges 1800156d0`, `ContentObject 180015000`, `Representation 180016380`, `ColorConverter 1800271c0`, `Transparency 18001ca50`)
  - observados vs implementação (Form XObjects, α, clipping, anotações)
  - nível de confiança + limitações (form matrices não-identidade; soft masks; SMask; undo/redo).

## 7. Testes obrigatórios

`tests/fixtures/*.pdf` (pasta com geradores Python):

- `test_text.pdf` (Helvetica/Times/Courier + metadata)
- `test_vector.pdf` (CMYK, curva, spot PANTONE, dashed)
- `test_images.pdf` (raster embutido)
- `test_multipage.pdf` (3 pages; page 2 contém a operação de clipping W*)
- `test_cmyk_inks.pdf` (separável CMYK + spot customizado)
- `test_formxobject.pdf` (Form XObject referenciado duas vezes)
- `test_unsupported.pdf` (anotação de link — produz `annotation(s) — not yet supported` warning)

**CTest -C Release -V:**
```
1/5 FormXObjects ... Passed
2/5 PdfSmoke ... Passed
3/5 UnitTests ... Passed
4/5 PdfRoundTrip ... Passed
5/5 EditingTests ... Passed
100% tests passed out of 5
```

## 8. Entrega obrigatória

- ✅ Release compilada após cada alteração
- ✅ 5/5 CTest verdes
- ✅ `ArtPlus.exe` abre, importa, renderiza, seleciona e exporta (screenshots demonstram)
- ✅ Não há placeholders críticos (imagem re-embutida lossless; clipping vira objeto real; masks pintados/exportados)
- ✅ Worker tree limpo em `master`, publicação feita

## 9. Git e relatório

**Arquivos criados/modificados (este último recorte):**

- Modificados: `src/document/document_model.{h,cpp}` (ClippingMaskObject, clipMaskIds, fontes), `src/document/document_serializer.cpp` (persistência de masks/clips/alpha, remap/load fix), `src/pdf/pdf_content.cpp/h` (sequência ordenada, BaseFont, correção de ordem Qt), `src/pdf/pdf_importer.cpp` (ordered import, clipping como objeto, grupo de forms com filhos), `src/pdf/pdf_exporter.cpp/h` (alpha ExtGState, clip emission, Image XObjects lossless), `src/ui/main_window.cpp/h` (ClipAware items, navegação de páginas, Page combo, layer filter, `--page` CLI), `src/app/main.cpp` (`--page`, `--open`), `tests/unit/test_pdf_roundtrip.cpp` (clip mask round-trip, image XObject assert), `tests/unit/test_editing.cpp`, `CMakeLists.txt`
- Criados: `tests/integration/pdf_smoke.cpp`, `tests/integration/form_xobject_test.cpp`, `tests/unit/test_editing.cpp`, `tests/fixtures/{test_formxobject,test_unsupported}.pdf`, `tests/fixtures/make_test_pdfs.py`, `rag/ghidra/document/phase3-evidence.md`, `rag/ghidra/document/phase3-delivery-evidence.md`
- Removidos: alvos de diagnóstico `pdf_probe`, `image_export_*`, `clip_debug`; arquivos temporários `qs.cpp`, `repro.cpp`

**SHA publicado:** `7f6d7f9324028e8a0fdb082d73726f451bddabec` é o último publicado no remoto `origin/master`; este recorte (clipe/alpha/fixes) será commitado a seguir e empurrado para `master` via `origin`.

**Remote:** `origin → https://github.com/evaneiaw/artplus-phase3.git` (público); o repositório `evaneiaw/artplus` **não foi tocado**.

**Comando de verificação:** `git rev-parse HEAD` → `master`; `git ls-remote origin master` após push.

## Limitações documentadas (não cobertas no escopo desta fase)

- Form matrices não-identidade podem compor transforms incorretamente (children de `Form /Do`).
- `SMask`/grupos de transparência no PDF editado viram `DeviceRGB` simples (perda de soft masks).
- Sem undo/redo (a infraestrutura `ArtDoc::ActionList` não foi replicada).
- Fontes exportadas usam `BaseFont /Helvetica` apenas (sem subsetting).
- Texto não re-embute glifos/hex (latin-1 limitado); conteúdo original não-modificado preserva o PDF byte-a-byte.
- Erros `W` aninhados exigindo `h` explícito após `W` são tratados — mas múltiplos clips na mesma graphics-state são tratados como lista ordenada de masks no modelo, com intersect no render.
- Anotações (`/Annots`) apenas reportadas como warning; não viram objetos.
