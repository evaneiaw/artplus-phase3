# REVERSE_ENGINEERING_FINDINGS.md

## Descobertas e Evidências da Análise Estática

1. **Arquitetura Base**:
   - O ArtPro+ v24 é uma aplicação C++ moderna de 64 bits para Windows.
   - Utiliza Qt 6 (`Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`) para o sistema de interface gráfica e renderização.
   - Depende de bibliotecas de terceiros padrão para geometria e tratamento de arquivos (`zlib`, `boost`, `libxml`).

2. **Separação de Motores**:
   - A arquitetura modular do executável de referência divide claramente as responsabilidades entre parsers de PDF (`PDFConversionRules.dll`), layout e imposição (`LayoutEngine.dll`), geometria/cantos (`Cornering.dll`) e renderização/UI (`ArtPro+.exe` + `Qt6`).

3. **Conclusões para o ArtPlus**:
   - O projeto independente **ArtPlus** adotará uma divisão equivalente em camadas (`src/pdf/`, `src/geometry/`, `src/production/`, `src/ui/`), garantindo que a implementação replique a robustez funcional do ecossistema sem qualquer reaproveitamento de código fechado.
