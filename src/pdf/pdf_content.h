#pragma once

#include "pdf_types.h"
#include "pdf_document.h"
#include "../document/document_model.h"
#include <QTransform>
#include <QString>
#include <vector>

namespace ArtPlus {
namespace PDF {

struct ParsedPath {
    struct Element {
        enum class T { Move, Line, Cubic, Close } t;
        QPointF p1, p2, p3;
    };
    std::vector<Element> elements;
    bool hasFill = false;
    bool hasStroke = false;
    Color fillColor;
    Color strokeColor;
    float lineWidth = 1.0f;
    Qt::PenCapStyle capStyle = Qt::FlatCap;
    Qt::PenJoinStyle joinStyle = Qt::MiterJoin;
    double miterLimit = 10.0;
    std::vector<float> dashPattern;
    float dashPhase = 0.0f;
    float fillAlpha = 1.0f;
    float strokeAlpha = 1.0f;
    bool evenOddFill = false;
};

struct ParsedText {
    QString text;
    QString fontName;
    float fontSize = 12.0f;
    Color fillColor;
    bool hasFill = true;
    QTransform transform;
    float fillAlpha = 1.0f;
};

struct ParsedImage {
    QString resourceName;
    int xObjectNumber = 0;
    int xObjectGeneration = 0;
    double displayWidth = 1.0, displayHeight = 1.0;
    QTransform transform;
    float opacity = 1.0f;
};

struct ClippedArea {
    std::vector<ParsedPath::Element> elements;
    bool evenOdd = false;
};

struct ParseResult {
    std::vector<ParsedPath> paths;
    std::vector<ParsedText> texts;
    std::vector<ParsedImage> images;
    std::vector<ClippedArea> clips;
    std::vector<QString> unsupported;
    bool hadError = false;
};

class PdfContentParser {
public:
    static ParseResult parse(const QByteArray& content,
                             const PdfDocument& doc,
                             const PdfValue& resources,
                             const QTransform& initialCtm,
                             double pageHeight);
private:
    PdfContentParser() = delete;
};

} // namespace PDF
} // namespace ArtPlus
