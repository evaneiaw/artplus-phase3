# Índice de Áreas Funcionais - Document.dll

Este índice mapeia os namespaces e classes identificados via análise estática (Ghidra).

## Estrutura de Document.dll

### Core Document Architecture
- `ArtDoc::ContentObject` / `ListItem`
- `PageRanges`
- `ReadOptions` / `WriteOptions`

### Color Management
- `ArtColor::ColorConverter` (e sub-classes para CMYK, RGB, Gray, Lab)
- `ArtColor::ICCProfile`
- `GenColorProfiles`

### Transparency & Blending
- `ArtBlend::Transparency`
- `ArtBlend::SoftMask`

### Undo/Redo & Actions
- `ActionList`, `Action`, `ActionGroup`
- `ChangeTracker`, `ErrorRecorder`

### Annotations
- `Annotation`, `Annotation3D`, `AnnotationText`, `Popup`

### Fase 3 — PDF Core Implementado
- `phase3-evidence.md` — cross-evidence entre Ghidra, ArtPro+ observável e o módulo `src/pdf/` do ArtPlus: importador, exportador, serialização `.artplus`, testes Editing/RoundTrip.
- `phase3-delivery-evidence.md` — confirmação 5/5 CTests, FormXObject groups, annotations warnings, endereços relevantes, limitações reconfirmadas.

---
*Generated: 2026-11-17*
*Base Path: rag/ghidra/exports/Document.dll/*
