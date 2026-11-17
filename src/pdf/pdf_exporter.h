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
// Otherwise the content is regenerated from the model and original
// unsupported constructs may be re-encoded; see limitations docs.
class PdfExporter {
public:
    static bool exportPdf(const Document& doc, const QString& outPath,
                          const QString& originalImportedPath = {});

private:
    // Low-level PDF construction helpers
    struct Output {
        std::vector<std::string> objects;   // indexed by object number-1
        std::vector<std::string> bodies;    // raw body for stream objects
        std::vector<bool> isStream;
        std::vector<int> streamLengths;

        // Append object, returns object number (1-based)
        int addObject(const std::string& body, bool stream = false, int streamLen = 0) {
            objects.push_back(body);
            isStream.push_back(stream);
            streamLengths.push_back(streamLen);
            return (int)objects.size();
        }
    };

    static QByteArray colorForOperator(const Color& c, bool stroke); // writes k/K, rg/RG, g/G or Separation cs
    static QByteArray escapeTextString(const QString& text);
    static QString pdfDateString(const QString& iso);
};

} // namespace PDF
} // namespace ArtPlus
