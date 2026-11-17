#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <QImage>
#include <QRectF>
#include <QTransform>
#include <QColor>
#include <QUuid>

namespace ArtPlus {
namespace PDF {

struct PDFPageInfo {
    int index = 0;
    QRectF mediaBox;
    QRectF cropBox;
    QRectF bleedBox;
    QRectF trimBox;
    QRectF artBox;
    float rotation = 0;
    bool valid = false;
};

struct PDFDocumentInfo {
    QString title;
    QString author;
    QString subject;
    QString keywords;
    QString creator;
    QString producer;
    QString creationDate;
    QString modificationDate;
    int pageCount = 0;
    bool tagged = false;
    bool encrypted = false;
};

enum class PDFObjectType {
    Path,
    Text,
    Image,
    Form,
    Group,
    Unknown
};

struct PDFTextRun {
    QString text;
    QString fontName;
    float fontSize = 0;
    QColor fillColor;
    QColor strokeColor;
    bool hasFill = true;
    bool hasStroke = false;
    QTransform transform;
    QRectF bbox;
};

struct PDFPathData {
    std::vector<uint8_t> commands;
    std::vector<float> points;
    QColor fillColor;
    QColor strokeColor;
    bool hasFill = false;
    bool hasStroke = false;
    float lineWidth = 1.0f;
    Qt::PenCapStyle capStyle = Qt::FlatCap;
    Qt::PenJoinStyle joinStyle = Qt::MiterJoin;
    std::vector<float> dashPattern;
    float dashPhase = 0;
    QTransform transform;
    int blendMode = 0;
    float opacity = 1.0f;
};

struct PDFImageData {
    QImage image;
    QRectF rect;
    QTransform transform;
    int blendMode = 0;
    float opacity = 1.0f;
    bool isMask = false;
    QColor maskColor;
};

struct PDFObject {
    PDFObjectType type = PDFObjectType::Unknown;
    QRectF bbox;
    QTransform transform;
    std::variant<PDFPathData, PDFTextRun, PDFImageData> data;
};

struct PDFPageContent {
    int pageIndex = 0;
    std::vector<PDFObject> objects;
    QRectF pageBox;
};

enum class RenderIntent {
    Display,
    Print,
    Proof
};

struct RenderOptions {
    float dpi = 72.0f;
    RenderIntent intent = RenderIntent::Display;
    bool renderAnnotations = true;
    bool renderForms = true;
    bool antiAlias = true;
    bool textAntiAlias = true;
    bool thinLineMode = false;
    QColor backgroundColor = Qt::white;
    QRectF clipRect;
    float rotation = 0;
};

class PDFEngine {
public:
    PDFEngine();
    ~PDFEngine();

    bool initialize();
    void shutdown();

    bool loadDocument(const QString& filePath, const QString& password = {});
    void closeDocument();

    bool isLoaded() const { return m_loaded; }
    const PDFDocumentInfo& documentInfo() const { return m_docInfo; }
    int pageCount() const { return m_docInfo.pageCount; }

    PDFPageInfo pageInfo(int index) const;

    PDFPageContent parsePage(int index);

    QImage renderPage(int index, const RenderOptions& options = {});

    bool saveDocument(const QString& filePath, bool incremental = false);

    std::vector<uint8_t> getPDFData() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_loaded = false;
    PDFDocumentInfo m_docInfo;
};

class PDFWriter {
public:
    PDFWriter();
    ~PDFWriter();

    bool createDocument();
    void addPage(const QRectF& mediaBox);
    void addPath(const PDFPathData& path);
    void addText(const PDFTextRun& text);
    void addImage(const PDFImageData& image);
    bool save(const QString& filePath);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class PDFParser {
public:
    static std::vector<PDFObject> parseContentStream(const std::vector<uint8_t>& stream, const QTransform& ctm = {});

private:
    static bool parsePath(const std::vector<uint8_t>& stream, size_t& offset, PDFPathData& out);
    static bool parseText(const std::vector<uint8_t>& stream, size_t& offset, PDFTextRun& out);
    static bool parseImage(const std::vector<uint8_t>& stream, size_t& offset, PDFImageData& out);
};

} // namespace PDF
} // namespace ArtPlus