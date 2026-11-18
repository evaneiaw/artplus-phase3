#pragma once

#include "pdf_types.h"
#include "pdf_document.h"
#include "../document/document_model.h"
#include <QTransform>
#include <QString>
#include <vector>

namespace ArtPlus {
namespace PDF {

struct SoftMaskRef {
    int objectNumber = 0;     // the referenced XObject (mask image or form)
    int objectGeneration = 0;
    QString mode;             // "Alpha" or "Luminosity"
    bool hasValue = false;
};

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
    SoftMaskRef softMask;
};

struct ParsedText {
    QString text;
    QString fontName;
    float fontSize = 12.0f;
    Color fillColor;
    bool hasFill = true;
    QTransform transform;
    float fillAlpha = 1.0f;
    SoftMaskRef softMask;
};

struct ParsedImage {
    QString resourceName;
    int xObjectNumber = 0;
    int xObjectGeneration = 0;
    double displayWidth = 1.0, displayHeight = 1.0;
    QTransform transform;
    float opacity = 1.0f;
    SoftMaskRef softMask;
};

struct ClippedArea {
    std::vector<ParsedPath::Element> elements;
    bool evenOdd = false;
};

// Ordered event stream for faithful model construction and grouping.
// indices refer to the paths/texts/images/clips vectors of ParseResult.
struct Event {
    enum class T { Path, Text, Image, Clip, Push, Pop } t;
    int index = -1;
};

struct ParseResult {
    std::vector<ParsedPath> paths;
    std::vector<ParsedText> texts;
    std::vector<ParsedImage> images;
    std::vector<ClippedArea> clips;
    std::vector<Event> sequence;
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
