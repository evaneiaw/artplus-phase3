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

---
*Generated: 2026-11-17*
*Base Path: rag/ghidra/exports/Document.dll/*
