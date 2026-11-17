# Color Management - Document.dll

## Overview
Gerenciamento completo de espaços de cor, conversões, perfis ICC e rendering intents.

## Namespaces/Classes Principais

### ArtColor::ColorConverter (Base)
- **Address**: `1800271c0`
- **Factory**: `ColorConverterFactory` / `RgbConverterFactory` / `GrayConverterFactory` / `LabConverterFactory` / `CmykConverterFactory`
- **Método chave**: `Convert(void, 5 params)` - conversão multi-thread

### Conversores Específicos
| Classe | Address | Params Factory |
|--------|---------|----------------|
| `CmykToCmykConverter` | `180021170` | 3 |
| `CmykToGrayConverter` | `180021340` | 3 |
| `CmykToLabConverter` | `180021510` | 3 |
| `CmykToRgbConverter` | `1800216e0` | 3 |
| `GrayToCmykConverter` | `18002c6a0` | 3 |
| `GrayToGrayConverter` | `18002c870` | 3 |
| `GrayToLabConverter` | `18002ca40` | 3 |
| `GrayToRgbConverter` | `18002cc10` | 3 |
| `LabToCmykConverter` | `18002eba0` | 3 |
| `LabToGrayConverter` | `18002ed70` | 3 |
| `LabToLabConverter` | `18002ef40` | 3 |
| `LabToRgbConverter` | `18002f110` | 3 |
| `RgbToCmykConverter` | `1800319f0` | 3 |
| `RgbToGrayConverter` | `180031bc0` | 3 |
| `RgbToLabConverter` | `180031d90` | 3 |
| `RgbToRgbConverter` | `180031f60` | 3 |

### ICC Profile Management
| Classe | Address | Descrição |
|--------|---------|-----------|
| `ICCProfile` | `18002da50` | Wrapper para perfis ICC |
| `GenColorProfiles` | `18002ac00` | Singleton de perfis genéricos |
| `PlatformProfileIterator` | `180031040` | Iterador perfis do sistema |

### GenColorProfiles - Métodos Singleton
- `Instance()` → `GenColorProfiles*` - `18002bf10`
- `GetDefaultGrayProfile()` → `shared_ptr<ICCProfile>` - `18002b930`
- `GetDefaultRgbProfile()` → `shared_ptr<ICCProfile>` - `18002bbf0`
- `GetDefaultCmykProfile()` → `shared_ptr<ICCProfile>` - `18002b8d0`
- `GetDefaultLabProfile()` → `shared_ptr<ICCProfile>` - `18002b990`
- `GetGenericCmykProfile()` → `shared_ptr<ICCProfile>` - `18002bcf0`
- `CacheProfile()` - `18002b530`, `18002b630`
- `Finalize()` - `18002b850`

### Perfis Embutidos (Strings)
```
GenXYZ.icm
GenGray.icm
GenLab.icm
sRGB_IEC61966-2-1_noBPC.icc
ISOcoated_v2_eci_300.icc
GenCMYK.icm
```

### Color Spaces Suportados (Strings)
```
DeviceCMYK
DeviceGray()
DeviceRGB()
CalGray(WhitePoint(...); BlackPoint(...); Gamma(...))
CalRGB(WhitePoint(...); Matrix(...))
Lab(WhitePoint(...); Range(...))
ICCGray(...)
ICCRGB(...)
ICCLab(...)
ICCCMYK(...)
```

### Rendering Intents
```
Perceptual
RelativeColorimetric
AbsoluteColorimetric
Default
```

### Evidence: Cross-references
- `GetColorConverter(3)` - `180028430` - factory centralizada
- `CreateICCProfile(2/3)` - `18002e470`, `18002e650`, `18002e7e0`
- `GetColorSpace()`, `GetData()`, `GetWhitePoint()`, `GetProfileID()`
- CMS.DLL imports: `TCMS_Init`, `TCMS_End`, `TInk_GetLabValue`, `TIDB_GetInk`

---
*Generated from: functions.csv, strings.csv, symbols.csv*