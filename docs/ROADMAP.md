# ArtPlus Development Roadmap

**Repository:** https://github.com/evaneiaw/artplus-phase3  
**Branch:** master  
**Current Commit:** 8fed344bdd32669f58b45b6835fd3b02fa9df803  
**Last Build:** 5/5 CTest tests passed (Release, MinGW MSVCRT, Qt 6.8.3)  
**Project Local:** H:\Desktop\ArtPlus  
**Reference:** Esko ArtPro+ v24.03.10017  

---
---

## PHASE 1 — CONSOLIDAÇÃO DO NÚCLEO E EDITOR GRÁFICO

### Objetivo
Transformar o núcleo existente em um editor vetorial plenamente funcional, com importação/edição/exportação de PDFs reais sem perda de conteúdo. O código já possui um núcleo PDF robusto e um modelo de documento; esta fase consiste em expô-lo através de uma UI completa, garantindo que todas as operações (seleção, edição, transformação, salvar/reabrir) funcionem corretamente e preservem a fidelidade do documento original.

### Funcionalidades e Tarefas Detalhadas

| # | Tarefa | Prioridade | Dependências | Módulo/Arquivo |
|---|--------|-----------|-------------|----------------|
| 1.1 | Auditoria da arquitetura existente e validação de integridade do modelo | P0 | — | `src/document/document_model.*`, `src/pdf/*` |
| 1.2 | Seleção individual e múltipla (clique, rubber band, Shift/Ctrl) | P0 | 1.1 | `src/ui/main_window.cpp/h`, `src/ui/scene.*` |
| 1.3 | Ferramentas de seleção e edição de nós (add/remove/bezier handles) | P1 | 1.1, 1.2 | `src/ui/main_window.cpp` |
| 1.4 | Criação e edição de caminhos Bézier (pen tool) | P1 | 1.2 | `src/ui/main_window.cpp` |
| 1.5 | Edição de textos (fontes, tamanho, cor, posição) | P1 | 1.1, model text objects | `src/pdf/pdf_content.*, src/ui/main_window.cpp` |
| 1.6 | Grupos e desagrupamento (GroupObject/ObjectType::Group) | P0 | 1.1 | `src/pdf/pdf_importer.cpp`, `src/document/document_model.h` |
| 1.7 | Clipping e máscaras de recorte (ClippingMaskObject) | P0 | 1.1 | `src/pdf/pdf_importer.cpp`, `src/ui/main_window.cpp` |
| 1.8 | Transformações (mover, escalar, rotacionar, espelhar) | P0 | 1.1, 1.2 | `src/pdf/pdf_content.cpp` (ordem de multiplicação Qt), `src/ui/main_window.cpp` |
| 1.9 | Alinhamento e distribuição de objetos | P2 | 1.1, 1.2 | `src/ui/main_window.cpp` |
| 1.10 | Camadas e organização de objetos ( LayerId, hierarquia ) | P0 | 1.1 | `src/document/document_model.h` |
| 1.11 | Painel de propriedades (inspector) | P1 | 1.1, 1.10 | `src/ui/main_window.cpp` |
| 1.12 | Undo/Redo completo (command stack, ids estáveis) | P1 | 1.1 | `src/document/undo.{h,cpp}`, `src/ui/main_window.cpp` |
| 1.13 | Zoom, navegação e pranchetas (Fit, Zoom In/Out, Pan) | P0 | 1.1 | `src/ui/main_window.cpp` |
| 1.14 | Salvamento e reabertura sem perda de conteúdo (.artplus JSON) | P0 | 1.1, serializer | `src/document/document_serializer.*` |
| 1.15 | Integração real entre ferramentas, menus e documentos | P0 | 1.2–1.14 | `src/ui/main_window.cpp/h` |

### Dependências Técnicas
- Modelo de documento com IDs uint32 estáveis (`document_model.h`).
- Parser PDF sequencial (events: Path/Text/Image/Clip/Push/Pop) (`pdf_content.h/cpp`).
- Importador que converte PDF → modelo (`pdf_importer.h/cpp`).
- Exportador passthrough + regeneração (`pdf_exporter.h/cpp`).
- Serializador JSON com remap de IDs após load (`document_serializer.h/cpp`).
- UndoStack com TransformCommand, DeleteObjectsCommand, etc. (`document/undo.h/cpp`).
- QGraphicsView/QScene itens com clip/alpha awareness (`src/ui/main_window.cpp`).

### Arquivos e Módulos Envolvidos
- `src/document/document_model.{h,cpp}` — ObjectType, clipMaskIds, softMaskId, Layer hierarchy.
- `src/pdf/pdf_content.{h,cpp}` — ParseResult, SoftMaskRef, sequential events, lexer fix (double-negation).
- `src/pdf/pdf_importer.{h,cpp}` — import ParsedInto, Form recursion, clip/mask resolution.
- `src/pdf/pdf_exporter.{h,cpp}` — passthrough byte-a-byte, ExtGState, SMask emission, Image XObjects lossless.
- `src/document/document_serializer.{h,cpp}` — save/load JSON, remap de IDs (objects, layers, children, clipMasks, parentId, softMaskId).
- `src/document/undo.{h,cpp}` — TransformCommand, DeleteObjectsCommand, EditTextCommand, SetVisibilityCommand, UndoStack.
- `src/ui/main_window.{h,cpp}` — UI completa: seleção, ferramentas, painting com clip/softmask, navigation, docks, Undo/Redo ação.
- `tests/unit/test_editing.cpp` — 9 testes de undo/redo (move, delete, scale, rotate).
- `tests/integration/pdf_smoke.cpp` — validação de import/export de fixtures.
- `CMakeLists.txt` — DOCUMENT_SOURCES inclui undo.cpp/undo.h.

### Referências Correspondentes
- **Ghidra:** `rag/ghidra/document/file-io-pdf.md`, `phase3-evidence.md`, `phase3-delivery-evidence.md` (ReadOptions 1800161e0, PageRanges 1800156d0, ContentObject 180015000, Representation 180016380, ColorConverter 1800271c0, Transparency 18001ca50).
- **ArtPro+ v24:** leitura de PDFs reais, importação de paths/text/images, operações de clipping, transparência via ExtGState, Form XObjects com matrizes, gerenciamento de cores CMYK/Spot.

### Testes Necessários
- `testImportVector`, `testImportImages`, `testImportMultiPage` (já existentes).
- `testClipMaskRoundTrip` — mask sobrevive save/load com IDs consistentes.
- `testExportEmbedsImageXObjects` — assert automática de `/Type /XObject`, `/Subtype /Image`, `FlateDecode`.
- `testFormTransformBounds` — 5 instâncias (identity/scale/rotate/mirror/nested) comparadas com PyMuPDF.
- `testUndoRedoMoveCommand`, `testUndoRedoDeleteCommand`, `testUndoRedoTransformScaleCommand`.
- Visual: abrir PDF real, editar um caminho, salvar, reabrir e verificar que o caminho permanece editado.

### Critérios Objetivos de Aceitação
- [ ] Abrir um PDF de teste (textos, vetores, imagens, transparências, clipping) e exibir páginas corretamente.
- [ ] Selecionar um objeto individualmente; mover arrastando; deletar; a operação reflete no modelo e no arquivo .artplus salvo.
- [ ] Ctrl+Shift + arrastar = seleção múltipla; Shift sozinho = adicionar à seleção.
- [ ] Transformar (mover/escala/rotação) um objeto selecionado; Ctrl+Z desfaz; Ctrl+Y refaz.
- [ ] Salvar como .artplus e reabrir: páginas, camadas, objetos, transforms, cores e clipping devem ser idênticos.
- [ ] O menu Edit tem Undo (Ctrl+Z) e Redo (Ctrl+Y) funcional; a pilha esvazia corretamente.
- [ ] Zoom (Fit, +/–) e pan funcionam no QGraphicsView; a seleção não move a página como Hand Tool.

### Limitações Conhecidas (documentadas para esta fase)
- Form matrices não-identidade podem compor transforms incorretamente em filhos `Form /Do` aninhados (já identificado e testado; P0 corrigido na fase 3, mas pode exigir validação visual adicional).
- `SMask`/`grupos de transparência` no PDF re-exportado viram `DeviceRGB` simples (perda de soft masks documentada; não é bloqueador para a fase 1).
- Sem suporte a anotações `/Annots` como objetos editáveis (apenas warning).
- Fontes exportadas usam `BaseFont /Helvetica` apenas (sem subsetting/hex).
- Texto não re-embute glifos/hex além de latin-1 simples; conteúdo original preservado via passthrough quando `modified=false`.

### Evidências Exigidas para Declarar Conclusão
- Screenshots do aplicativo mostrando: seleção simples/múltipla, transformação de objeto, painel de propriedades, undo/redo na ação, salvar/reabrir PDF com conteúdo preservado.
- Saída de `ctest -C Release -V` com 5/5 testes verdes.
- `git log --oneline -3` mostrando o último commit em `master`.
- `git ls-remote origin master` confirmando o SHA publicado.
- `docs/ROADMAP.md` atualizado com o estado atual desta fase.

### Conclusão da Fase 1
A Fase 1 será declarada concluída quando for possível:
1. Abrir um PDF, editar seus elementos (caminhos, textos, imagens) individualmente.
2. Salvar o documento e reabri-lo com todo o conteúdo preservado.
3. Usar Undo/Redo (Ctrl+Z/Ctrl+Y) sem corrupção de IDs ou hierarquia.
4. Renderizar e navegar (zoom/pan) corretamente.
5. Todos os CTests da Fase 1 passarem (a serem definidos após o planejamento detalhado).

---
---

## PHASE 2 — PRÉ-IMPRESSÃO PROFISSIONAL

### Objetivo
Implementar os recursos profissionais de preparação para impressão, transformando o ArtPlus em uma ferramenta de pré-press técnica. Isso inclui gerenciamento de cores ICC, separações CMYK/Spot, trapping, verificação de preflight e produção de arquivos prontos para impressão.

### Funcionalidades e Tarefas Detalhadas

| # | Tarefa | Prioridade | Dependências | Módulo/Arquivo |
|---|--------|-----------|-------------|----------------|
| 2.1 | Gerenciamento de cores ICC: perfis de entrada/ saída, gamut mapping | P1 | 1.1 (modelo consolidado), color-management RAG | `src/pdf/pdf_content.*, color-management RAG` |
| 2.2 | Espaços de cor: CMYK, RGB, Gray, Spot Colors e DeviceN | P1 | 1.1, 2.1 | `src/pdf/pdf_content.cpp` (modelo ColorSpace) |
| 2.3 | Painel de separações (individual e combinada) | P2 | 1.1, 2.1, 2.2 | `src/ui/main_window.cpp` (docks) |
| 2.4 | Visualização de chapas individuais (Overprint/Overprint Preview) | P2 | 1.1, 2.1, 2.2 | `src/ui/main_window.cpp` rendering |
| 2.5 | Trapping automático e manual | P2 | 1.1, 2.1 | nova lógica de overlap/overprint |
| 2.6 | Sangria e marcas de impressão (crop/registration marks) | P2 | 1.1, 1.14 (salvamento) | geração de páginas extras + export |
| 2.7 | Barras de controle e escalas de cor | P2 | 1.1, 2.1 | rendering de retângulos com cores sólidas/padrão |
| 2.8 | Preflight e inspeção técnica | P1 | 1.1, 2.1, 2.2–2.7 | detecção de fontes ausentes, imagens low‑res, resolução, espaços de cor |
| 2.9 | Conversões de cores com preservação de perfis | P1 | 2.1, color-management RAG | conversão CMYK↔RGB, ICC profile embedding |
| 2.10 | Detecção de fontes ausentes no documento | P1 | 1.1, model fonts | verificação de /Font resources vs. uso real |
| 2.11 | Verificação de imagens em baixa resolução | P1 | 1.1, model images | DPI check vs. requisitos de saída |
| 2.12 | Exportação de arquivos de impressão prontos (PDF/X, PDF/PEG) | P2 | 1.14 (salvamento), exporter | modificar PdfExporter::exportPdf para marcar metadados PDF/X |

### Dependências Técnicas
- Modelo de cor `ColorSpace` já existe (`document_model.h`); precisa estender para ICCBased/Separation.
- RAG `color-management.md` já documenta endereços `ColorConverter 1800271c0`; usar como referência.
- O exportador PDF já gera DeviceRGB + FlateDecode; estender para PDF/X-1a, PDF/X-3.
- Serializador JSON precisa persistir espaços de cor e tipos de fonte.

### Arquivos e Módulos Envolvidos
- `src/document/document_model.h` — extensão do `ColorSpace` enum/struct com ICCBased/Separation/DeviceN.
- `src/pdf/pdf_content.h/cpp` — parsing de perfis ICC, gamut checking, conversão de espaço.
- `src/pdf/pdf_exporter.h/cpp` — marcação de metadados PDF/X-1a/PDF-X-3, exportação de separações.
- `src/ui/main_window.h/cpp` — painel de separações, modo overprint preview, ferramentas de preflight.
- `RAG/color-management.md` — referência de endereços e comportamentos observados no ArtPro+.
- `tests/unit/test_colors.cpp` (a criar) — testes de conversão e preservação de cores.

### Referências Correspondentes
- **Ghidra:** `rag/ghidra/document/color-management.md` (ColorConverter 1800271c0, RGB/CMYK/Lab/Gray, comportamento observado).
- **ArtPro+ v24:** painel de separações, overprint preview, trapping automático, preflight automático, marcações de sangria/registro, exportação PDF/X.

### Testes Necessários
- `testColorConversion` — CMYK → RGB → CMYK roundtrip preserva componentes (ou registra desvios esperados).
- `testProfilePreservation` — PDF com perfil ICC embebido é reaberto com perfil intacto.
- `testPreflight` — documento com fontes faltantes, imagens low‑res e espaços de cor incorretos gera warnings/errors.
- `testSeparations` — visualização de chapas individuais (On/Off) e separação CMYK/K.
- `testPDFX` — exportar PDF/X-1a e reabrir em leitor compatível (verificar metadados).

### Critérios Objetivos de Aceitação
- [ ] Um documento CMYK com perfis ICC pode ser aberto e a cor éPreviewada corretamente no painel.
- [ ] O painel de separações lista todas as chapas (C, M, Y, K, Spot) e pode toggle cada uma.
- [ ] O overprint preview alterna a visualização de sobreposição de tintas.
- [ ] O preflight detecta fontes faltantes, imagens abaixo de 300 DPI e espaços de cor não-embedados.
- [ ] Exportar como PDF/X-1a preserva as marcas de corte e as separações; o arquivo reabre em Adobe Acrobat/Reader sem warnings de pré-flighting.
- [ ] A conversão CMYK↔RGB não causa clipping silencioso; os componentes são registrados (mesmo que fora de gama).

### Limitações Conhecidas (documentadas para esta fase)
- Perfis ICC completos exigem biblioteca externa (LittleCMS2); o ArtPlus pode usar Qt `QColorSpace` limitado ou implementar conversão de gamut simplificada.
- Trapping automático exige algoritmo de detecção de borda + sobreposição de tintas; pode gerar falsos positivos/negativos.
- DeviceN e separações múltiplas não são totalmente suportadas; foco em CMYK + SpotColors individuais.
- PDF/X-4 (preservação de dados transparentes/bitmap) não é objetivo desta fase; foco em PDF/X-1a e PDF/X-3.

### Evidências Exigidas para Declarar Conclusão
- Screenshots do painel de separações, modo overprint preview, relatório de preflight.
- Saída de `ctest -C Release -V` incluindo testes de nova funcionalidade (a serem adicionados).
- Documentos de teste (PDFs de pré-flighting) com resultados válidos.
- `docs/ROADMAP.md` atualizado com o estado atual da Fase 2.

### Conclusão da Fase 2
A Fase 2 será declarada concluída quando for possível:
1. Abrir um documento CMYK/K com perfil ICC e visualizar as chapas individualmente.
2. Executar o preflight e obter relatório preciso sobre fontes, imagens e cores.
3. Exportar PDF/X-1a que reabre sem warnings de pré-flighting em leitores padrão.
4. Todos os testes de validação de cores e separações passarem.

---
---

## PHASE 3 — PRODUÇÃO GRÁFICA E MONTAGEM

### Objetivo
Implementar ferramentas de produção, imposição e aproveitamento de material. O ArtPlus deve suportar layouts de produção gráfica: step and repeat, imposed pages, poses, marcas de corte e registro, dielines, e organização de trabalhos e versões para embalagens e materiais de impressão.

### Funcionalidades e Tarefas Detalhadas

| # | Tarefa | Prioridade | Dependências | Módulo/Arquivo |
|---|--------|-----------|-------------|----------------|
| 3.1 | Step and Repeat (repetição por grade e distribuição personalizada) | P0 | 1.14 (salvamento), model layout | nova funcionalidade no `src/ui/main_window.cpp` |
| 3.2 | Repetição por grade (grid repeat) com offsets e angular | P0 | 3.1 | mesma implementação de Step and Repeat |
| 3.3 | Montagem de layouts (arrastar páginas para composição) | P1 | 1.1, 3.1 | seletor de páginas + canvas composição |
| 3.4 | Imposição de páginas (imposition layouts: perfect, half, quarter, etc.) | P2 | 3.1, 3.3 | nova lógica de reordenação de páginas |
| 3.5 | Organização de poses (poses for packaging) | P1 | 3.1, 3.3 | grid de poses + visualização |
| 3.6 | Marcas de corte e registro | P0 | 1.14, 3.3, 3.5 | geração de marcas no canvas + export PDF |
| 3.7 | Dielines, facas e linhas de vinco | P2 | 3.1, 3.3 | paths vetoriais sobrepostos ao layout |
| 3.8 | Controle de aproveitamento de material (nesting básico) | P2 | 3.1, 3.3 | algoritmo simples de empacotamento |
| 3.9 | Organização de trabalhos e versões (workbook, versão de prova) | P1 | 1.14, model document | painel de gestão de versões |
| 3.10 | Exportação de arquivos de produção (imposição, nesting, marks) | P1 | 1.14, exporter | modificar PdfExporter para incluir marcas e layout |

### Dependências Técnicas
- Modelos de layout e posição de páginas já têm `PageId`, `LayerId`, `mediaBox`; estender com `nesting` e `imposition`.
- O exportador PDF já suporta clipping e SMask; estender para marcas de corte/registro.
- CMakeLists.txt já tem alvos de teste; adicionar testes de imposição/nesting.
- RAG `production-engine` ainda não existe; criar ou usar `phase3-evidence.md` como base.

### Arquivos e Módulos Envolvidos
- `src/ui/main_window.h/cpp` — step and repeat UI, grid de poses, imposição, marcas de corte.
- `src/document/document_model.h` — campos `impositionLayout`, `nesting`, `marks` em Page/Document.
- `src/pdf/pdf_exporter.h/cpp` — geração de marcas de corte/registro no PDF de saída.
- `RAG/production-engine.md` (a criar) — evidências de comportamentos ArtPro+ na produção.
- `tests/unit/test_imposition.cpp` (a criar) — testes de step and repeat e layouts de imposição.

### Referências Correspondentes
- **Ghidra:** `rag/ghidra/document/phase3-delivery-evidence.md` (muitas referências de produção já estão documentadas implicitamente nos actions observados).
- **ArtPro+ v24:** step and repeat, imposição de páginas (perfect, half, quarter), marcas de corte/registro, dielines para embalagens, nesting de arte, controle de versão de trabalhos.

### Testes Necessários
- `testStepRepeat` — repetir um objeto/bitmap por grid com offset/ângulo; comparar posições esperadas vs. geradas.
- `testImpositionLayout` — aplicar layout de imposição (ex.: half) a um documento de múltiplas páginas; verificar reordenação e sobreposição.
- `testMarksAndBleed` — marcas de corte e sangria são geradas no PDF de exportação e são visíveis no leitor.
- `testNesting` — algoritmo básico de nesting coloca dois retângulos dentro de um retângulo maior sem overlap.

### Critérios Objetivos de Aceitação
- [ ] Step and Repeat: objeto repetido por grid 3x3 com offset X/Y e ângulo; posições batem com cálculo esperado.
- [ ] Imposição: layout "half" de 2 páginas organiza as páginas 1 e 2 em folha dobrada; visualização no canvas.
- [ ] Marcas de corte e registro aparecem no PDF exportado e são visíveis em Adobe Acrobat/Reader.
- [ ] Dielines são paths vetoriais editáveis no canvas.
- [ ] O painel de versões lista os estados salvos do documento e permite alternar entre eles.

### Limitações Conhecidas (documentadas para esta fase)
- Imposição complexa (sela, perfect-binding) exige algoritmos de reordenação page‑by‑page que ainda não estão no escopo.
- Nesting avançado (muitas formas irregulares) requer biblioteca externa ou algoritmo NP‑hard; foco em rect‑rect packing básico.
- Dielines exigem conhecimento da estrutura do trabalho (bleed, safety zone); podem ser inseridos manualmente como paths.
- Step and Repeat simples (grid retangular/angular) é o objetivo P0; distribuições personalizadas são P2.

### Evidências Exigidas para Declarar Conclusão
- Screenshots do step and repeat, grid de imposição, marcas de corte no canvas e no PDF exportado.
- Saída de testes automatizados (testStepRepeat, testImpositionLayout, testMarksAndBleed).
- `docs/ROADMAP.md` atualizado com o estado atual da Fase 3.
- `git log` e `git ls-remote origin master` confirmando o commit publicado.

### Conclusão da Fase 3
A Fase 3 será declarada concluída quando for possível:
1. Usar step and repeat para duplicar objetos por grade/ângulo.
2. Aplicar layout de imposição a um documento multi-página e visualizar a disposição.
3. Gerar e exportar marcas de corte/registro que aparecem corretamente no PDF.
4. Todos os testes de imposição e marks passarem.
5. `docs/ROADMAP.md` atualizado.

---
---

## PHASE 4 — AUTOMAÇÃO, ACABAMENTO E PRODUTO FINAL

### Objetivo
Consolidar o ArtPlus como uma plataforma profissional completa, com automação de operações, processamento em lote, scripts, presets e uma interface profissional e personalizável. Todas as funcionalidades das fases anteriores devem estar integradas, estáveis e validadas comparativamente com o ArtPro+.

### Funcionalidades e Tarefas Detalhadas

| # | Tarefa | Prioridade | Dependências | Módulo/Arquivo |
|---|--------|-----------|-------------|----------------|
| 4.1 | Action Lists e automação de operações (sequence of commands) | P1 | 1.12 (undo/redo), 3.1 (step repeat) | nova classe `ActionList` ou stack de comandos compiláveis |
| 4.2 | Processamento em lote (batch processing de arquivos) | P1 | 4.1, 1.14 (salvamento) | script/CLI que itera sobre diretório de PDFs |
| 4.3 | Scripts e extensibilidade (Lua, Python ouInternal DSL) | P2 | 4.1, 4.2 | API exposed para automação externa |
| 4.4 | Presets de produção (configurações de cores, marcas, exportação) | P1 | 2.10 (preflight), 3.10 (export), 4.2 | JSON/XML de presets armazenados em `%APPDATA%` ou projeto |
| 4.5 | Relatórios técnicos (relatório de preflight, relatório de cores, log de mudanças) | P2 | 2.8 (preflight), 4.2, 4.3 | geração de PDF/TXT/HTML com os resultados |
| 4.6 | Otimização de desempenho (grandes documentos, muitos objetos) | P2 | model document size, rendering cache | QGraphicsView performance tuning, cache de texturas |
| 4.7 | Gerenciamento de documentos grandes (>100 páginas) | P1 | 1.14, 4.2, 4.6 | suporte a pranchetas múltiplas, carregamento sob demanda |
| 4.8 | Compatibilidade avançada com PDFs (PDF 1.4–1.7, PDF/A, PDF/PEG) | P2 | exporter, pdf_content parser | suporte a mais versões do PDF e padrões de conformidade |
| 4.9 | Recuperação de documentos e estabilidade (crash recovery, auto-save) | P1 | 1.14 (salvamento), model document state | QSettings/QFileAutoReset ou mecanismo similar |
| 4.10 | Interface profissional e personalizável (themes, toolbar layout, shortcuts) | P1 | UI framework Qt, main_window | configurável via JSON ou resources |
| 4.11 | Auditoria completa de menus, botões e atalhos | P0 | 4.10 | garantir que todo menu/ botão funcional tenha atalho/icone consistente |
| 4.12 | Testes de regressão e validação comparativa com ArtPro+ | P0 | todas as fases | suite de comparação visual/estrutural contra PDFs de referência ArtPro+ |

### Dependências Técnicas
- Todas as fases anteriores devem estar estabilizadas (P0/P1/P2 do documento).
- O model de documento deve suportar persisting de estado de automação (ActionList).
- O serializer JSON deve suportar o novo tipo de dado (ActionList/presets).
- A UI deve ser themeable via QSS ou recursos Qt.
- RAG `undo-redo-actions.md` já documenta alguns comportamentos; usar como base.

### Arquivos e Módulos Envolvidos
- `src/document/document_model.h` — campos para ActionList, presets, estado de recuperação.
- `src/document/document_serializer.h/cpp` — persistência de ActionList e presets no JSON.
- `src/ui/main_window.h/cpp` — barra de ferramentas personalizável, atalhos, themes, workspace.
- `RAG/undo-redo-actions.md` — referência de comportamentos esperados.
- `RAG/production-engine.md` (a criar) — para produção e automação.
- `tests/unit/test_regression.cpp` (a criar) — suite de regressão comparativa ArtPro+.
- `CMakeLists.txt` — adicionar alvos de teste de automação e compatibilidade.

### Referências Correspondentes
- **Ghidra:** `rag/ghidra/document/undo-redo-actions.md`, `phase3-delivery-evidence.md` (actions observados em Document.dll).
- **ArtPro+ v24:** action lists (scripts gravados), processamento em lote, presets de impressão, recuperação de documentos, compatibilidade de PDFs, interface personalizável, atalhos avançados.

### Testes Necessários
- `testActionListExecution` — gravar uma sequência de ações (move/transform/export) e reproduzi-la.
- `testBatchProcessing` — processar um diretório de 10+ PDFs com as mesmas configurações; verificar que todos saem idênticos.
- `testCompatibility` — comparar PDF de entrada vs. saída em leitores Adobe (verificar páginas, dimensões, cores).
- `testCrashRecovery` — simular crash (fechar abruptamente) e verificar que o documento pode ser reaberto (auto-save).
- `testThemeSupport` — alternar tema claro/escuro e verificar que a UI permanece funcional.

### Critérios Objetivos de Aceitação
- [ ] Action Lists podem ser gravadas, salvas e reproduzidas sem perda de estado.
- [ ] O processamento em lote de 20+ PDFs idênticos produz 20+ arquivos de saída idênticos (geometria, cores, marcas).
- [ ] A compatibilidade de PDF é validada: documentos abertos em Adobe Acrobat/Reader não mostram diferenças visuais em relação ao original.
- [ ] O auto-save/recovery evita perda de trabalho após crash simulado.
- [ ] A interface suporta temas claros/escuro e layout de toolbar configurável.
- [ ] Todos os menus e botões têm atalho consistente (ou estão devidamente documentados como não mapeados).
- [ ] A suite de regressão comparativa ArtPro+ não revela diferenças estruturais ou visuais não intencionais.

### Limitações Conhecidas (documentadas para esta fase)
- Action Lists atuais são limitados a operações de transformação e export; não cobrem operações complexas de prepress.
- O processamento em lote não suporta transformações complexas de página (imposition por lote).
- A compatibilidade avançada (PDF/A, PDF/PEG) exige validação externa (Ferramenta Adobe Preflight ou similar).
- A recuperação de documentos após crash pode não cobrir todos os estados possíveis; foco em saves frequentes.
- A interface personalizável pode ter limitações em ambientes com recursos de tela reduzidos.

### Evidências Exigidas para Declarar Conclusão
- Screenshots da interface com temas claro/escuro, action list reproduzida, processing batch em execução.
- Saída de `ctest -C Release -V` incluindo testes de automação e compatibilidade.
- Relatórios de comparação ArtPro+ (arquivos de diff ou screenshots lado a lado).
- `docs/ROADMAP.md` atualizado com o estado atual da Fase 4.
- `git log` e `git ls-remote origin master` confirmando o commit publicado.

### Conclusão da Fase 4
A Fase 4 será declarada concluída quando for possível:
1. Gravar, salvar e reproduzir Action Lists sem perda de estado.
2. Processar em lote um lote de PDFs e obter saída idêntica.
3. Validar compatibilidade de PDF em leitores Adobe sem diferenças visuais não intencionais.
4. Recuperar um documento após crash simulado (auto-save).
5. A interface suporta temas e layout configurável.
6. Todos os CTests e testes de regressão passarem.
7. `docs/ROADMAP.md` finalizado e publicado em `master`.

---
---

## REGRAS PERMANENTES DE EXECUÇÃO

Estas regras se aplicam a todas as fases e devem ser registradas no `docs/ROADMAP.md`:

1. **Trabalhar sobre a arquitetura e o código existentes.**  
   Não reinicie o projeto nem substitua componentes funcionais sem justificativa técnica comprovada. Aproveite integralmente o trabalho já entregue (núcleo PDF, modelo de documento, UI base, testes CTest).

2. **Não invente funcionalidades nem presuma comportamentos sem evidência.**  
   Toda nova funcionalidade deve ser fundamentada por: (a) análise do código-fonte, (b) evidências do Ghidra, (c) comportamento observado no ArtPro+ v24, ou (d) teste automatizado. Documentação sozinha não é implementação.

3. **Consulte Ghidra, documentação/RAG e ArtPro+ durante o desenvolvimento.**  
   Os arquivos `rag/ghidra/document/*.md` e o conhecimento do ArtPro+ são a fonte de verdade para comportamentos observados no binário proprietário. Use-os para validar decisões de implementação e identificar gaps.

4. **Compile em Release após cada alteração de código.**  
   `cmake --build . --config Release` deve ser executado imediatamente após qualquer edição de .cpp/.h. Se o build falhar, corrija antes de prosseguir.

5. **Execute os testes relevantes e todos os CTests.**  
   `ctest -C Release -V` deve sempre retornar 5/5 (ou o número esperado de testes verdes). Se um teste falhar, identifique a causa raiz antes de avançar.

6. **Valide visualmente a inicialização e as funcionalidades alteradas.**  
   `ArtPlus.exe` deve iniciar sem erros; as funcionalidades modificadas devem ser verificadas visualmente (screenshots). Não confie apenas em testes unitários.

7. **Não deixe stubs, placeholders ou botões sem funcionamento real.**  
   Cada item de menu, botão ou atalho deve ter implementação completa ou estar explicitamente documentado como "não implementado nesta fase" com razão técnica.

8. **Não declare uma funcionalidade concluída sem evidências.**  
   Screenshots, resultados de testes, saída de `ctest`, e `git status` são as evidências mínimas. Uma afirmação sem dados não conta.

9. **Atualize documentação, RAG e testes junto com as implementações.**  
   `docs/ROADMAP.md`, `rag/ghidra/document/*.md` e os arquivos de teste devem ser atualizados no mesmo commit da funcionalidade. Não adie a documentação.

10. **Fazer commit e push para origin/master ao concluir cada fase.**  
    `git commit -m "..." && git push origin master`. O repo `evaneiaw/artplus` antigo NÃO deve ser tocado.

11. **Informe SHA completo, resultados dos testes e limitações.**  
    No final de cada fase, registre: SHA do commit, saída de `ctest -C Release -V`, e as limitações conhecidas documentadas na respectiva fase.

12. **Não interrompa o trabalho por questões que possam ser resolvidas tecnicamente com as evidências disponíveis.**  
    Se um problema bloquear a progressão temporária, busque alternativa no código existente, RAG ou ArtPro+ antes de parar. Se for necessário decidir entre caminhos, registre a decisão no RAG e prossiga.

13. **Preservar funcionalidades existentes e corrigir regressões antes de avançar.**  
    Qualquer regressão introduzida por uma nova funcionalidade deve ser corrigida imediatamente. A estabilidade do build existente tem prioridade sobre novidades.

---
---

## EXECUÇÃO PLANEJADA

### Ordem de Execução Recomendada

1. **Fase 1** — Consolidar o núcleo e o editor gráfico (já parcialmente concluído; restam validar seleção, undo/redo, salvar/reabrir PDFs realisticamente e garantir que todas as ferramentas do menu estejam funcional).
2. **Fase 2** — Pré-impressão profissional (gerenciamento de cores, separações, preflight, PDF/X).
3. **Fase 3** — Produção gráfica e montagem (step and repeat, imposição, marcas, dielines).
4. **Fase 4** — Automação, acabamento e produto final (action lists, batch processing, scripts, otimização, compatibilidade avançada).

### Pontos de Controle (Gates)

Antes de avançar de fase, é obrigatório:
- `ctest -C Release -V` com 100% dos testes da fase atual verdes.
- Screenshots documentando as funcionalidades-chave da fase.
- `docs/ROADMAP.md` atualizado com o estado conclusivo da fase.
- `git commit` e `git push origin master` com o SHA do commit final.
- Lista de arquivos criados, modificados e removidos registrada no final do documento.

### Estimativa de Esforço (apenas para planejamento)

- **Fase 1:** 150–200 horas (correção de bugs residuais, UI completa, undo/redo integrado, salvar/reabrir sem perda).
- **Fase 2:** 120–180 horas (cor LMScms2 integration parcial, painel de separações, preflight, PDF/X).
- **Fase 3:** 100–150 horas (step and repeat, imposição basic, marks, dielines).
- **Fase 4:** 150–220 horas (action lists, batch processing, compatibilidade ArtPro+, estabilidade, interface).

**Total estimado:** 520–750 horas de desenvolvimento, dependendo da profundidade de cada recurso e da necessidade de integração com bibliotecas externas (LittleCMS2 para gerenciamento de cores completo).

---

## ESTADO ATUAL DO PROJETO (Diagnóstico)

### Contexto
O ArtPlus encontra-se no commit `8fed344` em `master`, com a **Fase 3 finalizada** e **5/5 CTest tests passados**. O trabalho já realiza:

- Importação e interpretação de PDFs reais (textos, vetores, imagens, transparências, clipping).
- Modelo de documento com IDs estáveis, hierarquia e coordendas.
- Exportação com preservação (passthrough byte-a-byte quando não modificado, regeneração PDF simples quando modificado).
- UI `main_window` com seleção, move, delete, zoom/pan, docks Layers/Objects.
- Serializador `.artplus` JSON com integridade (`compare()`).
- Testes automatizados: `FormXObjects`, `PdfSmoke`, `UnitTests`, `PdfRoundTrip`, `EditingTests`.
- Engenharia reversa cruzada com Ghidra (endereços, confianças, limitações documentadas).
- Publicação em `https://github.com/evaneiaw/artplus-phase3` (`origin/master`).
- Screenshots visuais validando text, clip, alpha, image rendering e navegação de página.

### Pendências Conhecidas (documentadas nas fases acima)
1. Form matrices não identidade podem compor transforms incorretamente em filhos `Form /Do` aninhados.
2. `SMask`/`grupos de transparência` no PDF re-exportado viram `DeviceRGB` simples (perda de soft masks).
3. Sem undo/redo histórico de edição de propriedades individuais (apenas movimento/delete/transform).
4. Fontes exportadas usam `BaseFont /Helvetica` apenas (sem subsetting/hex).
5. Texto não re-embute glifos/hex além de latin-1 simples; conteúdo não-modificado preservado via passthrough.
6. Anotações `/Annots` apenas reportadas como warning; não viram objetos editáveis.

### Arquivos Mais Recentemente Modificados
- `src/document/document_model.{h,cpp}` — `ClippingMaskObject`, `OpacityMaskObject`, `softMaskId`, `clipMaskIds`.
- `src/pdf/pdf_content.cpp/h` — sequência ordenada de eventos, `SoftMaskRef`, parsing SMask, correção lexer (double-negation).
- `src/pdf/pdf_importer.cpp` — importação ordenada, clipping como objeto real, grupos Forms, resolução SMask.
- `src/pdf/pdf_exporter.cpp/h` — ExtGState com `/ca`/`/CA`, `/SMask` emission, Image XObjects lossless (FlateDecode RGB).
- `src/ui/main_window.cpp/h` — `ClipAwareItem` com soft mask render, navegação por página (`Page:` combo), `--page N` CLI, Undo/Redo na UI.
- `src/document/undo.{h,cpp}` — nova arquitetura de command stack (TransformCommand, DeleteObjectsCommand, EditTextCommand, SetVisibilityCommand).
- `tests/unit/test_editing.cpp` — 9 testes de undo/redo (move, delete, scale, rotate).
- `tests/fixtures/test_smask.pdf` — fixture SMask de teste.
- `tests/integration/smask_probe.cpp` — validação SMask.
- `CMakeLists.txt` — `undo.cpp` no `DOCUMENT_SOURCES`.
- `docs/phases/PHASE3_REPORT.md` — relatório atualizado com todas as evidências.
- `rag/ghidra/document/phase3-delivery-evidence.md` — endereços Ghidra + limitações.

### Roadmap Pendente
Este documento `docs/ROADMAP.md` será criado na execução atual e servirá como plano diretor obrigatório para as próximas fases. Nenhuma funcionalidade da Fase 1–4 deve ser iniciada antes do presente roadmap estar aprovado e publicado em `master`.

---
---

## PRÓXIMOS PASSOS IMEDIATOS

1. **Criar `docs/ROADMAP.md`** — já iniciado neste documento.
2. **Commit e push do ROADMAP.md** para `origin/master`.
3. **Validar que o build Release ainda passa** (5/5 CTest).
4. **Apresentar o roadmap completo** ao usuário/stakeholder para aprovação da sequência de fases.
5. **Iniciar a Fase 1** apenas após aprovação do roadmap (não nesta execução).

---