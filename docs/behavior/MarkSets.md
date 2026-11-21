# Mark Sets — captura comportamental BLOQUEADA

## Estado da captura

**BLOQUEIO (2026-11-19):** captura comportamental em ArtPro+ v25 proibida pelo orquestrador; evidência comportamental pendente de definição de alvo alternativo.

Nenhuma CAP foi gerada para F-002. Não há comportamento observado a declarar. A spec `docs/features/F-002.md` e qualquer ADR derivado **não** podem usar este arquivo como evidência comportamental.

## Correlato estático (já existente, não comportamental)

O fluxo estático em `rag/evidence/F-001/ghidra/DocumentMarks__0x18024D3E0.md` (confiança alta) descreve `TaskMarkSets::ApplyMarksFromSet` / `ApplyRelevantMarkSets` / `CopyMarksToPageWithLog` / `PrepareMarksCachePage` / `MakeFormsFromMarks`, mas nenhum desses pontos foi provado ou refutado por UI nesta sessão:

- (a) cópia da primeira página de cada documento do mark set — **não verificado**;
- (b) o que é copiado (conteúdo de página, marcas, camadas) — **não verificado**;
- (c) mesclagem de separações/informes de tinta — **não verificado**;
- (d) `MakeFormsFromMarks` cria forms — **não verificado**.

## Artefatos da tentativa interrompida

- `tools/reference_capture/F-002/shot.ps1` — helper local de captura (utilitário, não evidência).
- `tools/reference_capture/F-002/{input,output,shots}/` — diretórios criados, vazios. O único screenshot v25 inicial (`shot-00-initial.png`) foi **removido** para não constituir CAP de v25.
- `rag/evidence/F-002/behavior/manifest.json` — bloqueio registrado; `comportamental: []`.

## sessão GUI

Ao receber a correção do orquestrador, a sessão GUI foi interrompida sem operações de clique/teclado no ArtPro+: a janela v25 (`test_vector.pdf @ 97,52%* – ArtPro+`) estava aberta com alterações não salvas preexistentes (do F-001), nenhum diálogo pendente. Nenhum documento foi modificado ou salvo por esta sessão.

## Capturas

Nenhuma. `CAP-20261119-03` **não** foi emitido.
