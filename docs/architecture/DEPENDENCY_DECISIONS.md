# DEPENDENCY_DECISIONS.md

## Análise de Dependências

- **PDF**: PDFium, PoDoFo, qpdf.
- **Geometria**: Clipper2, Boost.Geometry, CGAL.
- **Renderização**: Qt RHI / Skia.
- **Texto**: HarfBuzz, FreeType.
- **Imagens**: OpenImageIO, OpenCV.
- **Código de Barras**: ZXing.
- **Testes**: GoogleTest, Qt Test.
- **Banco de Dados**: SQLite.

Cada dependência será avaliada conforme as diretrizes do Security License Auditor antes da integração.
