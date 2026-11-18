#pragma once

#include "pdf_document.h"
#include "pdf_content.h"
#include "../document/document_model.h"
#include <QImage>
#include <QMap>
#include <vector>

namespace ArtPlus {

struct PdfImportResult {
    int pagesImported = 0;
    int objectsImported = 0;
    int pathsImported = 0;
    int textsImported = 0;
    int imagesImported = 0;
    std::vector<QString> warnings;     // unsupported features retained verbatim or skipped
    bool ok = false;
    QString error;
};

// Converts an open PdfDocument into the ArtPlus document model.
// Each page becomes a Page with its own layer; objects from the PDF are
// converted to Path/Text/Image objects where supported.
class PdfImporter {
public:
    static PdfImportResult import(const PDF::PdfDocument& doc, Document& out);

private:
    static Color cmykToRgbColor(const Color& cmyk);
    static QRectF pathBounds(const std::vector<PDF::ParsedPath::Element>& elements);
};

} // namespace ArtPlus
