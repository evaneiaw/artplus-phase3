# Transparency & Blending - Document.dll

## Overview
Sistema de transparência, blend modes, soft masks e knockout/isolation groups.

## Namespaces/Classes Principais

### ArtBlend::Transparency
- **Factory**: `Factory` (3 params - `18001c560`, `18001c5e0`)
- **Construtor completo**: `Transparency(10 params)` - `18001ca50`
- **Destrutor**: `~Transparency` - `18001cfb0`

#### Propriedades (Getters/Setters via Factory)
| Método | Address | Tipo |
|--------|---------|------|
| `GetBlendMode()` | `18001e6b0` | `BlendMode` |
| `SetBlendMode(Factory*, 2)` | `18001e940` | - |
| `GetFillAlpha()` | `18001e820` | `Component` |
| `SetFillAlpha(Factory*, 2)` | `18001e950` | - |
| `GetStrokeAlpha()` | `18001e840` | `Component` |
| `SetStrokeAlpha(Factory*, 2)` | `18001ea20` | - |
| `IsIsolated()` | `18001e910` | `bool` |
| `SetIsolated(Factory*, 2)` | `18001e960` | - |
| `IsKnockout()` | `18001e920` | `bool` |
| `SetKnockout(Factory*, 2)` | `18001e970` | - |
| `IsKnockoutShape()` | `18001e930` | `bool` |
| `SetKnockoutShape(Factory*, 2)` | `18001e980` | - |
| `GetRI()` | `18001e830` | `RenderingIntent` |
| `SetRI(Factory*, 2)` | `18001e990` | - |
| `SetSoftMask(Factory*, 2)` | `18001e9a0` | - |
| `Get()` - shared_ptr | `18001dc10`, `18001e470`, `18001e690` | 3 overloads |

#### Blend Modes (Strings - `1802e4a00+`)
```
Normal, Multiply, Screen, Overlay, SoftLight, HardLight,
ColorDodge, ColorBurn, Darken, Lighten, Difference, Exclusion,
Saturation, Color, Luminosity, <inherit from parent>
```

### ArtBlend::SoftMask
- **Construtor**: `SoftMask(7 params)` - `1800164a0`
- **Destrutor**: `~SoftMask` - `180017110`
- **Get**: `GetBackdropColor()`, `GetBackdropLuminosity()`, `GetOutsideAlpha()`
- **Set**: `SetBackdropColor()`, `SetIsAlpha()`, `SetTransferFunction()`, `SetTransformation()`

### TransferFunctionSet
- **Construtor**: `TransferFunctionSet(2 params)` - `1800165e0`

### Propriedades de Objetos (Strings para Debug)
```
Knockout / NotKnockout
Isolated / NotIsolated
fill alpha: X
stroke alpha: X
SoftMask(Index N)
TransferFunctionSet: Default
!!! NON GROUP OBJECT WITHOUT TRANSPARENCY PTR !!!
```

### Group Transparency (Content Object)
Strings indicam hierarquia:
```
Group colorspace: ...
Transparency(...) <see above> Default ...
ImageMask
Knockout
Isolated
```

### Evidence: Cross-references
- `DefiningSoftMask()` - `180028610` - retorna `shared_ptr<SoftMask_const>`
- `FillIsNormalOpaque()` - `18001dbe0`
- `StrokeIsNormalOpaque()` - `18001ea30`
- `GetConvertedColor()` - `18001e6c0` - `ColorList<ColorPolicy>*`
- `ContentTransformation()` - `180029c60`, `180042b40` - `TransformationBase<YAGL::MilliPointCoordinateSystem>*`

---
*Generated from: functions.csv, strings.csv*