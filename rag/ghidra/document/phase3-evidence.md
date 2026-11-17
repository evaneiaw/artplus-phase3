# Fase 3 — PDF Core e Importador/Exportador ArtPlus

## Escopo
Implementação independente de leitura/interpretação de PDF e da exportação para o modelo ArtPlus, sem copiar código Esko/Enfocus proprietário. Evidências extraídas de `functions.csv`, `strings.csv`, `symbols.csv` e de testes de importação/exportação.

## Evidências Ghidra usadas para orientar a arquitetura

| Área | Endereço / String | Evidência | Confiança | Aplicação no ArtPlus |
|---|---|---|---|---|
| DocumentLayer init | `Start DocumentLayer (0/7)` → `Install default content post readers (7/7)` | String gerada pelo `DocumentLayer` ctor/dtor | Alta | Replicamos uma sequência de inicialização equivalente: `PdfDocument::open()` → `parseAll()` → coleta de páginas |
| ReadOptions / WriteOptions | `1800161e0` / `180016730` | Ct/Dtor/Operator= das classes | Alta | `Document::read/write` em `PdfImporter`/`PdfExporter` |
| PageRanges | `1800156d0` | Ctor com 2 params | Média | `doc.pageOrder()` + travessia pela árvore `/Pages` → `/Kids` |
| ContentObject | `180015000` / `180016a40` | ListItem template — herança de lista ordenada | Média | `Document::objects()` (unordered_map) + `DocumentObject::children()` (vetor) |
| Representation | `180016380` | RGB/CMYK/Lab get/set + `Enrich()` | Média | `Color` suporta `Type::{CMYK,RGB,Gray,Lab,Spot,Pattern,None}`; conversão na importação |
| Can-séttes de cor / ColorConverter | `1800271c0` | Factory/singleton de conversores CMYK/RGB/Lab/Gray | Alta | Importador preserva componentes CMYK/Spot originais quando possível; fallback para `DeviceRGB` no exportador |
| ICC / Perfis embutidos | `18002da50` / `18002ac00` | Perfis GenCMYK.icm, sRGB_IEC61966-2-1_noBPC.icc, ISOcoated_v2_eci_300.icc | Alta | Não embutimos perfis ICC nativamente; imagem exportada fica em `DeviceRGB` com BitsPerComponent=8 |
| Transparency | `18001ca50` | Factory + 10 params; blend modes | Média | `/GS` ExtGState reconhecido (alpha `/ca`, `/CA`); soft masks não preservados no export; documentado |
| ActionList / Undo | `18003ed10` | AddAction/Undo/Redo/GetPendingAction | Alta | Fora do escopo desta fase (limitação documentada) |
| Annotations | `180051250` | Accept/Clone Visitor | Alta | Não importamos anotações ainda; prevê `ObjectType::ClippingMask`/`OpacityMask` no modelo |
| File I/O / handlers | Strings `application/pdf`, `application/illustrator` | Registro de handlers | Alta | `QFileDialog` filtra por `*.pdf`; extenso para `.artplus`/`.ait` em fase futura |
| Fonts / subsets / CID | Strings `Font(; subset)`, `Font caching done` | Cache de fontes | Média | Exportador usa `BaseFont /Helvetica` apenas (limitação); glifos não embedded |
| Metadata XMP | Strings `PDFAVersion`, `GTS_PDFXVersion`, `GTS_PDFVTVersion` | Namespaces Esko | Média | Reconhecidas como strings; não persistidas no `.artplus` ainda |

## Comportamento observado no ArtPro+ v24 (referência qualitativa, não instrumentalmente verificada)

- Abertura de PDFs preserva o arquivo quando não há edição (satisfeito por nossa exportação byte-a-byte quando `modified == false`).
- Editor exibe camadas como árvore e permite seleção individual/em grupo de objetos (replicado via `QGraphicsItem::setSelected()` + `Document::selectObject(id, additive)`).
- Objetos podem ser movidos arrastando (replicado em `ArtPlusScene::mouseMoveEvent()`), alterando `DocumentObject::m_transform`.
- Documentos com separações nativas CMYK/Spot mantêm os componentes (reproduzido via importador que preserva `Color::type=CMYK|Spot`).
- Transparência/soft masks são preservadas no arquivo original e **não** re-aplicadas na exportação modificada — mesma limitação que originalmente o próprio ArtPro+ documenta (soft masks muitas vezes são descartados em fluxos de trabalho simples de pré-impressão).
- Elementos não suportados (Form XObjects, anotações, links) são ignorados pelo importador mas **não corrompem** o modelo: caso contrário o arquivo exportado teria layout incorreto. Nossa política: registrar warning e manter renderização básica.
- O ArtPro+ exibe HUD e barras de ferramentas com funcionalidades editoriais (Select/Zoom/Pan/Delete). Nosso toolbar cobre as mesmas categorias funcionais.

## Confiança e limitações

- **Confiança:** Alta para importação legível de PDF moderno (conjunto de fixtures); Média para fidelidade de saída após modificação (cores Spot/CMYK viram DeviceRGB no export); Baixa para equivalência estrutural com ArtPro+ v24 (não replicamos Form XObjects, blend modes, anotações, media alignment/font subsetting interno).
- **Limitações:** sem Undo/Redo; sem soft masks; sem extração de ordem de desenho híbrida (intercalação de inline images e text operators não é totalmente suportada); o exportador não garante validação Acrobat/PDF-X.

## Ações tomadas nesta fase

1. Implementado parser PDF independente compatível com os fixtures de teste do projeto (`tests/fixtures/*.pdf`).
2. Integrado ao modelo `ArtPlus::Document` — IDs estáveis, camadas por página, seleção/movimentação/delete/restore de imagens embodied como XObjects `Image` / `DCTDecode` / `DeviceRGB`.
3. CTest cobre UnitTests (modelo), PdfRoundTrip (round-trip), EditingTests (seleção/move/scale/rotate/delete/reimport).
4. Documentação em `docs/phases/PHASE3_REPORT.md`; índices RAG atualizados; commit local único até aqui.

## Verificação (CTest)

```
1/3 UnitTests ... Passed
2/3 PdfRoundTrip ... Passed
3/3 EditingTests ... Passed
```

## Liênções cruzadas com outros arquivos RAG

- `document-architecture.md` → Base do modelo de documento
- `color-management.md` → Tratamento de cor (CMYK/RGB/Spot) na saída
- `transparency-blending.md` → Limites de suporte a blend modes
- `file-io-pdf.md` → Fluxos de abrir/salvar/exportar
- `undo-redo-actions.md` → Limites de não implementação nesta fase
