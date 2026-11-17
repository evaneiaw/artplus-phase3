# Document Architecture - Document.dll

## Overview
Core document model: páginas, content objects, opções de leitura/escrita, representações.

## Namespaces/Classes Principais

### ArtDoc::ContentObject & ListItem
| Função | Address | Tipo |
|--------|---------|------|
| `ListItem<ArtDoc::ContentObject>` ctor | `180015000` | 1 param |
| `~ListItem<...>` | `180016a40` | void, 1 param |

### PageRanges
- **Ctor**: `PageRanges(2 params)` - `1800156d0`
- **Dtor**: `~PageRanges` - `180016dd0`
- **Operator=**: `180017bf0`

### ReadOptions / WriteOptions
| Classe | Ctor | Dtor | Operator= |
|--------|------|------|-----------|
| `ReadOptions` | `1800161e0`, `1800162b0` (2 ctor) | `1800170a0` | `180017c60`, `180017dc0` |
| `WriteOptions` | `180016730` | `1800171c0` | - |

### Representation
- **Ctor**: `Representation(2 params)` - `180016380`
- **Dtor**: `~Representation` - `180032d90`
- **Operator==**: `180032e90`
- **Ctor (1 param)**: `180032c60`

#### Representation - Propriedades (Getters/Setters)
| Método | Address | Retorno |
|--------|---------|---------|
| `IsEmpty()` | `180033b80` | `bool` |
| `HasRgb()` | `180033b70` | `bool` |
| `HasCmyk()` | `180033b60` | `bool` |
| `HasLab()` | `180033b80` | `bool` |
| `GetRgb()` | `1800339f0` | `vector<Component>*` |
| `GetCmyk()` | `180033200` | `vector<Component>*` |
| `GetLab()` | `180033360` | `vector<Component>*` |
| `SetRgb()` | `180033d50` | void |
| `SetCmyk()` | `180033c30` | void |
| `SetLab()` | `180033cc0` | void |
| `SetXMPRGB()` | `180033de0` | void |
| `SetAlternateColorSpaceCMYK()` | `180033bb0` | void |
| `GetRepresentation()` | `1800335f0` | `Representation*` |
| `Enrich()` | `180033130` | void |

### Properties (Document Properties)
- **Ctor**: `Properties(2 params)` - `180016180`
- **Dtor**: `~Properties` - `180016d60`
- **Operator=**: `180017c20`

### Security
- **Ctor**: `Security(2 params)` - `1800163e0`, `180016450` (2 ctor)
- **Dtor**: `~Security` - `1800170f0`
- **Operator=**: `180017e60`, `180017ea0`

### Document Layer Init Sequence (Strings)
```
Start DocumentLayer (0/7)
Font caching done (1/7)
Enfocus PDFLib initialized (2/7)
initialized (3/7)
BGCMS
Mark/PieceInfo readers and writers installed (4/7)
Reuse Form queries installed (5/7)
File handlers registered (6/7)
Install default content post readers (7/7)
```

### Document Destruction Sequence
```
Scratch document destroyed (1/4)
destroyed (2/4)
Enfocus PDFLib finalized (3/4)
Font caching finalized (4/4)
```

### Document State Strings
```
Document: Open read/write: ...
Document: Open read-only: ...
Document: Exception caught while opening ...
Document: Destructor for ...
Document: Deleting working file ...
STATUS_SAVE
STATUS_OPTIMIZING
STATUS_WRITING_TO_DISK
Document: Could not copy the incremental base file to ...
Document: Switching to working file ...
Document: File has no pages
```

### Boxes & Orientation (Debug Strings)
```
Boxes: Media, Bleed
Orientation: ...
Crosshair: ...
Annotations: ...
```

### Evidence: Cross-references
- `Content::UserScaling`, `Content::IsBeingParsed`, `Content::PostReaderPhase`
- `Esko_Distortion`, `EskoPNGr`, `EskoPNPDFO`, `EskoPNBrix`
- `PieceInfo`, `DPartRoot`, `Perms`
- `PDFAVersion`, `PDFAConformance`, `GTS_PDFXVersion`, `GTS_PDFVTVersion`
- Metadata: `Author`, `Subject`, `Keywords`, `Title`, `Producer`, `Creator`, `ModDate`, `CreationDate`

---
*Generated from: functions.csv, strings.csv*