# DOCUMENT_MODEL_SPECIFICATION.md

## 1. Visão Geral da Arquitetura do ArtPlus

A arquitetura do ArtPlus é projetada para ser um modelo documental de alto nível, independente, mas compatível com o formato PDF+ da Esko. Ela se baseia nos padrões observados durante a engenharia reversa do `Document.dll`.

### 1.1. Princípios de Design
- **Gerenciamento de Memória**: Uso extensivo de `std::shared_ptr` e `std::weak_ptr` para replicar o comportamento do `GSharedObject` (Smart Pointers).
- **Padrão de Visitação (Visitor Pattern)**: Implementação de métodos `accept()` em todos os objetos gráficos para processamento desacoplado (renderização, exportação, análise).
- **Abstração de Objetos Gráficos (GO)**: Todos os elementos herdam de uma classe base comum `GOObject`.

## 2. Especificação de Classes Principais (C++)

### 2.1. GOObject (Base)
Classe abstrata fundamental para todos os elementos do documento.
- **Atributos**:
  - `Matrix3x3 transform`: Matriz de transformação local.
  - `GraphicState state`: Atributos de preenchimento, contorno e transparência.
  - `Rect boundingBox`: Caixa delimitadora calculada.
  - `std::weak_ptr<GOObject> parent`: Referência para o container pai.
- **Métodos**:
  - `virtual void accept(GOVisitor& visitor) = 0;`
  - `virtual std::shared_ptr<GOObject> clone() const = 0;`

### 2.2. GOPage (Página)
Container principal que organiza o conteúdo visual.
- **Atributos**:
  - `std::vector<Rect> boxes`: MediaBox, TrimBox, BleedBox, etc.
  - `double rotation`: Rotação da página.
  - `std::vector<std::shared_ptr<GOLayer>> layers`: Lista ordenada de camadas.

### 2.3. GOLayer (Camada)
Agrupador lógico de objetos, suportando hierarquia e metadados de visibilidade.
- **Atributos**:
  - `std::string name`: Nome da camada.
  - `bool isVisible`, `bool isLocked`, `bool isPrintable`.
  - `std::vector<std::shared_ptr<GOObject>> children`: Objetos contidos na camada.

### 2.4. GOPath (Caminho Geométrico)
Representa elementos vetoriais.
- **Atributos**:
  - `std::vector<SubPath> paths`: Coleção de caminhos (pontos e curvas Bézier).
  - `FillType fillType`: EvenOdd ou NonZero.
  - `bool isStroked`, `bool isFilled`.

### 2.5. GOText (Texto)
Representa elementos tipográficos.
- **Atributos**:
  - `std::string textContent`: O texto bruto.
  - `FontDescriptor font`: Informações de fonte e estilo.
  - `double fontSize`, `double horizontalScale`.

## 3. Modelo de Cor e Separação

### 3.1. ArtColor::ColorSpec
Estrutura para definição de cor.
- **Tipos**: `DeviceCMYK`, `DeviceRGB`, `DeviceGray`, `Lab`, `SpotColor` (Inks).
- **Atributos**:
  - `std::vector<double> components`: Valores das componentes de cor.
  - `std::shared_ptr<ArtInk> ink`: Referência para tinta spot, se aplicável.

### 3.2. ArtInk (Tinta/Separação)
- **Atributos**:
  - `std::string inkName`, `std::string bookName`.
  - `Lab labValue`: Valor de referência Lab para visualização.
  - `InkType type`: Normal, Opaque, Transparent, Finishing.

## 4. Metadados PDF+ (XMP)

O ArtPlus deve implementar o suporte ao namespace XMP da Esko: `http://ns.esko-graphics.com/pdfnatversion/1.0/`.
- **Chaves Críticas**:
  - `egGr:Inks`: Definição de separações customizadas.
  - `egGr:Layers`: Hierarquia de camadas persistida no PDF.
  - `egBarc:Barcodes`: Definições de códigos de barras.

## 5. Próximos Passos para Implementação
1. Criar as interfaces de cabeçalho (`.h`) em `src/core/model/`.
2. Implementar o motor de transformação de matrizes.
3. Integrar com uma biblioteca PDF de baixo nível (como o Poppler ou uma abstração própria para o PDFLib) para E/S.
4. Desenvolver o sistema de Undo/Redo baseado no Command Pattern.
