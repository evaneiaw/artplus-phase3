#pragma once

#include "../document/document_model.h"
#include <QString>
#include <map>
#include <vector>
#include <memory>

namespace ArtPlus {
namespace PDF {

// Writes the ArtPlus document model back to a PDF file.
// If the document was never modified since import, the original file is
// copied byte-identically (perfect preservation of unmodified content).
// Otherwise the content is regenerated from the model with the same visual
// fidelity for paths/text/images (RGB/gray/CMYK/spot colors, line styles,
// dash patterns). Lossy re-encoding applies only to images (none — Flate
// lossless RGB is used). See limitations docs for details.
class PdfExporter {
public:
    static bool exportPdf(const Document& doc, const QString& outPath,
                          const QString& originalImportedPath = {});

private:
    static QByteArray escapeTextString(const QString& text);
    static QString pdfDateString(const QString& iso);
    static QByteArray colorForOperator(const Color& c, bool stroke);
    // Lossless Flate RGB image bytes for an image XObject
    struct ImageEmbedding {
        int objectNum = 0;           // XObject object number
        int width = 0, height = 0;
        std::vector<uint8_t> rgbData; // w*h*3 raw bytes
    };
    static ImageEmbedding prepareImage(const QImage& img);
    static std::vector<uint8_t> flateCompress(const std::vector<uint8_t>& raw);
};

} // namespace PDF
} // namespace ArtPlus
