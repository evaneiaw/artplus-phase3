# Annotations - Document.dll

## Overview
Gerenciamento de anotações no documento, incluindo texto, 3D e popups.

## Namespaces/Classes Principais

### ArtDoc::Annotation
- **Ctors**: `Annotation(3 params)` - `180051250`, `Annotation(5 params)` - `180051410`
- **Dtor**: `~Annotation` - `180051b20`
- **Operator=**: `operator=,Annotation*,2` - `1800523e0`

#### Métodos Principais
| Método | Address | Retorno/Params |
|--------|---------|----------------|
| `Accept(void, 2)` | `180052100` | Implementa Visitor Pattern |
| `Clone(Annotation*, 2)` | `1800521a0` | Clona a anotação |
| `Clone(unique_ptr<ArtDoc::Annotation,...>, 2)` | `180052270` | Clona a anotação (unique_ptr) |

### ArtDoc::Annotation3D
- **Ctors**: `Annotation3D(3 params)` - `180051030`, `Annotation3D(6 params)` - `180051110`
- **Dtor**: `~Annotation3D` - `180051a90`

#### Métodos Principais
| Método | Address | Retorno/Params |
|--------|---------|----------------|
| `Accept(void, 2)` | `1800520b0` | Implementa Visitor Pattern |
| `Clone(Annotation3D*, 2)` | `180052150` | Clona a anotação 3D |

### ArtDoc::AnnotationText
- **Ctors**: `AnnotationText(3 params)` - `180051570`, `AnnotationText(6 params)` - `1800515c0`
- **Dtor**: `~AnnotationText` - `180051c20`

#### Métodos Principais
| Método | Address | Retorno/Params |
|--------|---------|----------------|
| `Accept(void, 2)` | `180052100` | Implementa Visitor Pattern |
| `Clone(AnnotationText*, 2)` | `1800521f0` | Clona a anotação de texto |

### ArtDoc::Popup
- **Ctors**: `Popup(3 params)` - `180051730`, `Popup(2 params)` - `1800517d0`
- **Dtor**: `~Popup` - `180051d00`

### Propriedades de Anotação (Debug Strings)
```
Annotations: ...
Text("...")
U3D(...)
TM ...
PC ...
AN(...)
AV(...)
ExternalName: "..."
InternalName: "..."
Background((...)
FOV ...
Orthographic
Perspective
Projection(...)
```

### Evidence: Cross-references
- `CContext::Accept` methods indicate visitor pattern implementation for different content objects, including annotations.
- Strings like `UNKNOWN TRANSFORMABLE`, `UNKNOWN CLIPPABLE`, `UNKNOWN BLENDABLE`, `UNKNOWN FILLABLE`, `UNKNOWN STROKABLE` suggest that annotations, like other content objects, can have transformation, clipping, blending, fill, and stroke properties.

---
*Generated from: functions.csv, strings.csv, symbols.csv*