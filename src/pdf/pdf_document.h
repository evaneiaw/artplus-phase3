#pragma once

#include "pdf_types.h"
#include <QByteArray>
#include <QString>
#include <QFile>
#include <map>
#include <memory>
#include <vector>

namespace ArtPlus {
namespace PDF {

// Information about one page of the document (PDF units: 1/72 inch).
struct PageGeometry {
    double mediaBox[4] = {0, 0, 595.0, 842.0}; // llx lly urx ury (PDF user space)
    double cropBox[4] = {0, 0, 595.0, 842.0};
    double bleedBox[4] = {0, 0, 595.0, 842.0};
    double trimBox[4] = {0, 0, 595.0, 842.0};
    double artBox[4] = {0, 0, 595.0, 842.0};
    double rotation = 0; // degrees
    double width() const { return cropBox[2] - cropBox[0]; }
    double height() const { return cropBox[3] - cropBox[1]; }
};

struct DocumentInfo {
    QString title;
    QString author;
    QString subject;
    QString keywords;
    QString creator;
    QString producer;
    QString creationDate;
    QString modificationDate;
    int pageCount = 0;
};

class PdfDocument {
public:
    PdfDocument() = default;
    ~PdfDocument() = default;

    bool open(const QString& filePath);
    bool isOpen() const { return m_open; }
    QString filePath() const { return m_path; }

    int pageCount() const { return (int)m_pageRefs.size(); }
    const DocumentInfo& info() const { return m_info; }

    // Resolve a value that may be a direct object or an indirect reference
    PdfValue resolve(const PdfValue& value) const;

    PdfValue pageDictionary(int pageIndex) const;
    PageGeometry pageGeometry(int pageIndex) const;
    PdfValue pageResources(int pageIndex) const;
    QByteArray pageContents(int pageIndex) const; // decoded content stream

    const PdfObject* findObject(int obj, int gen) const;
    const std::map<PdfObjectId, PdfObject>& allObjects() const { return m_objects; }
    std::vector<uint8_t> originalBytes() const { return std::vector<uint8_t>(m_bytes.begin(), m_bytes.end()); }
    int64_t startXrefOffset() const { return m_startXref; }
    const std::vector<PdfObjectId>& pageRefs() const { return m_pageRefs; }

private:
    QByteArray m_bytes;
    QString m_path;
    bool m_open = false;
    DocumentInfo m_info;
    int64_t m_startXref = 0;
    std::map<PdfObjectId, PdfObject> m_objects;
    std::vector<PdfObjectId> m_pageRefs;

    void parseAll(); // full-file "N G obj ... endobj" scan (robust for typical PDFs)
    void parseXref(int64_t offset);
    void collectPages(const PdfValue& node, int depth = 0);

    mutable std::map<PdfObjectId, PdfValue> m_resolvedCache;
    PdfValue resolveValue(const PdfValue& v) const;
    std::vector<uint8_t> decodeStream(const std::map<std::string, PdfValue>& dict, const QByteArray& raw) const;
    static std::vector<uint8_t> ascii85Decode(const QByteArray& in);
    static std::vector<uint8_t> asciiHexDecode(const QByteArray& in);
    static std::vector<uint8_t> flateDecode(const QByteArray& in);
    static QByteArray eolAt(const QByteArray& data, int64_t offset);
};

} // namespace PDF
} // namespace ArtPlus
