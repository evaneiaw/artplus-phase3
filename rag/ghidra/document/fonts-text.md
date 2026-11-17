# Fonts & Text - Document.dll

## Overview
Gerenciamento de fontes, subsetting, métricas, WMode (vertical/horizontal) e manipulação de texto.

## Classes e Métodos Principais

### ArtFont::Font (Shared From This)
- **Base**: `enable_shared_from_this<ArtFont::Font>` - `18001c2c0`
- **Dtor**: `~enable_shared_from_this<ArtFont::Font>` - `18001cda0`
- **shared_from_this**: `18001f8c0`
- **weak_from_this**: `1800201b0`

### Font Caching
```
Font caching done (1/7)
Font caching finalized (4/4)
```

### Font Attributes & Properties (Strings)
```
Font(; hyphen ...; space ...; style ...; family ...
; not-present
; embedded
; external
; subset
; vertical wmode
; no edit license
; glyph transform ...
; descent ...
; Ascent ...
; FontBBox ...
```

### Text Editing & Layout (Strings)
```
Text(...) bytes)
Text transformation: ...
Text size: ...
Horizontal scaling: ...
Spacing:
  char ...
  word ...
  level ...
Leading: ...
TextEditing
```

### Evidence: Cross-references
- `XMPlFont` e `SetFonts`, `GetFonts` em `XMPlib`.
- O sistema gerencia fontes incorporadas, externas e subconjuntos (*subset*), com suporte a WMode vertical (`vertical wmode`) e licenças de edição (`no edit license`).

---
*Generated from: functions.csv, strings.csv, symbols.csv*