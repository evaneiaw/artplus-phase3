# DOCUMENT_DATA_STRUCTURES.md

## 1. Abstrações de Alto Nível (Graphics Objects - GO)

A análise de strings e símbolos identifica uma hierarquia de classes prefixadas com `GO` (Graphics Object), que formam o núcleo do modelo de memória.

### 1.1. GOObject (Base)
Estrutura fundamental da qual herdam todos os elementos gráficos.
- **Campos Observados**: Matrix (Transformação), GraphicState, BoundingBox.
- **Métodos Virtuais**: `Accept` (Visitor Pattern), `CreateClone`, `GetObjectType`.

### 1.2. GOPath (Caminho Geométrico)
Representa vetores, curvas Bézier e polígonos.
- **Componentes**: `CCurve`, `SubPath`.
- **Atributos**: `IsStroked`, `IsFilled`, `FillType` (EvenOdd/NonZero).

### 1.3. GOText (Texto e Tipografia)
Representa elementos de texto editáveis ou convertidos.
- **Atributos**: `Font`, `FontSize`, `Leading`, `HorizontalScale`, `WordSpacing`.
- **Estruturas relacionadas**: `TextGroup`, `TextRun`.

### 1.4. GOPage (Página)
Container principal de objetos.
- **Page Boxes**: MediaBox, CropBox, BleedBox, TrimBox, ArtBox.
- **Propriedades**: Rotation, UserUnit, PageRanges.

## 2. Estruturas de Cor e Mistura

O motor de cores é robusto e lida com separações complexas e transparências.

### 2.1. ArtColor::ColorSpec
Estrutura que define uma cor no documento.
- **Tipos de Paint**: `FlatPaint`, `AxialShadingPaint`, `RadialShadingPaint`, `Pattern`.
- **Conversão**: Utiliza `ColorConverter` e perfis ICC (`GenCMYK.icm`, `sRGB.icc`).

### 2.2. Transparency & SoftMask
- **Modos de Mesclagem (BlendModes)**: Normal, Multiply, Screen, Overlay, SoftLight, HardLight, ColorDodge, ColorBurn, Darken, Lighten, Difference, Exclusion, Saturation, Color, Luminosity.
- **Propriedades**: `IsIsolated`, `IsKnockout`, `FillAlpha`, `StrokeAlpha`.

## 3. Estruturas de Metadados PDF+ (Proprietárias)

O ArtPlus deve replicar essas chaves para manter compatibilidade com arquivos "Normalizados".

- **Namespace**: `http://ns.esko-graphics.com/pdfnatversion/1.0/`
- **Chaves XMP**:
  - `Esko:Colorants`: Informação de separações e tintas spot.
  - `Esko:Barcode`: Definições de códigos de barras (magnificação, redução de largura de barra).
  - `Esko:ProcSteps`: Metadados para Finishing (Cutting, Creasing, Foil).
  - `GTS_PDFXVersion`: Compatibilidade PDF/X.

## 4. Notas de Implementação para ArtPlus
As estruturas devem ser implementadas como classes C++ com semântica de valor ou através de smart pointers (`std::shared_ptr`), refletindo o uso pesado de `GSharedObject` observado no binário original.
