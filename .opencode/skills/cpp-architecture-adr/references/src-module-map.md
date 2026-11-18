# Mapa de módulos atual

## `src/document/`

- `document_model.h/.cpp`: `Document`, `Page`, `PageBoxes`, `Layer`, `DocumentObject`, `PathObject`, `TextObject`, `ImageObject`, `GroupObject`, `ClippingMaskObject`, `OpacityMaskObject`, `Color`, `FillStroke`, `Gradient`.
- `undo.h/.cpp`: `Command`, `TransformCommand`, `DeleteObjectsCommand`.
- `document_serializer.h/.cpp`: persistência.

## `src/pdf/`

- `pdf_types.h/.cpp`: `PdfValue`, `PdfObject`, `PdfError`.
- `pdf_lexer`, `pdf_parser`, `pdf_document`, `pdf_content`.
- `pdf_importer`: `PdfImporter::import`.
- `pdf_exporter`: `PdfExporter` (cor, texto, imagem, Flate).

## `src/prepress/`

- `trapping.h/.cpp`: `TrappingEngine`.
- `marks.h/.cpp`: `MarksEngine` + structs de cada marca.
- `step_repeat.h/.cpp`: `StepRepeatEngine`.
- `imposition.h/.cpp`: `ImpositionEngine`.
- `packaging.h/.cpp`: `PackagingEngine` (dieline, cut/crease).

## `src/color/`

- `color_manager.h`: `ColorManager`, conversões, spot, DeviceN, simulação, DeltaE.
- `color_profile.h`: parser ICC.
- `color_transform.h`: transformação.

## `src/geometry/`

- `geometry_engine.h`: `Point`, `BezierCurve`.
- `path_operations.h`: booleanos, offset, simplify, smooth, join, split, retrace, cut, spread/choke, Douglas-Peucker.

## `src/automation/`

- `action_list.h/.cpp`: `ActionListEngine`.
- `scripting`, `performance`, `recovery`.

## `src/ui/`

- `main_window.h/.cpp`: janela principal.

## `src/app/`

- `main.cpp`: entry point.

## Alvos CMake

`ArtPlus` = documento + PDF + prepress + cor + automação + UI + app.
Testes linkam apenas o subconjunto necessário (documento + PDF, na maioria).

## Dependências permitidas

- Domínio (`document`, `pdf`, `prepress`, `geometry`, `color`): QtCore/QtGui, sem Widgets.
- UI: QtWidgets/Qml, pode chamar domínio.
- PDF: `Qt6::ZlibPrivate` para Flate.
