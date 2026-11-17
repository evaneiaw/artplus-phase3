#include "pdf_content.h"
#include "pdf_parser.h"
#include <QPainterPath>
#include <QtMath>

namespace ArtPlus {
namespace PDF {

// ========================================================= small PS evaluator
static std::vector<double> evaluateTintFunction(const QByteArray& psCode, double input, int expectedOutputs) {
    std::string code = psCode.toStdString();
    std::vector<double> stack;
    stack.push_back(input);

    size_t i = 0;
    auto isWS = [](char c) { return std::isspace((unsigned char)c) || c == '{' || c == '}'; };

    auto parseNumber = [&]() -> double {
        size_t s = i;
        while (i < code.size() && (std::isdigit((unsigned char)code[i]) || code[i] == '.' || code[i] == '-' || code[i] == '+')) i++;
        return std::stod(code.substr(s, i - s));
    };

    while (i < code.size()) {
        char c = code[i];
        if (isWS(c)) { i++; continue; }
        if (std::isdigit((unsigned char)c) || c == '-' || c == '+' || c == '.') {
            double v = parseNumber();
            stack.push_back(v);
            continue;
        }
        size_t s = i;
        while (i < code.size() && !isWS(code[i])) i++;
        std::string op = code.substr(s, i - s);

        if (op == "dup") { if (!stack.empty()) stack.push_back(stack.back()); }
        else if (op == "exch") {
            if (stack.size() >= 2) {
                size_t n = stack.size();
                std::swap(stack[n - 1], stack[n - 2]);
            }
        }
        else if (op == "pop") { if (!stack.empty()) stack.pop_back(); }
        else if (op == "add") {
            if (stack.size() >= 2) { double b = stack.back(); stack.pop_back(); double a = stack.back(); stack.pop_back(); stack.push_back(a + b); }
        }
        else if (op == "sub") {
            if (stack.size() >= 2) { double b = stack.back(); stack.pop_back(); double a = stack.back(); stack.pop_back(); stack.push_back(a - b); }
        }
        else if (op == "mul") {
            if (stack.size() >= 2) { double b = stack.back(); stack.pop_back(); double a = stack.back(); stack.pop_back(); stack.push_back(a * b); }
        }
        else if (op == "div") {
            if (stack.size() >= 2) { double b = stack.back(); stack.pop_back(); double a = stack.back(); stack.pop_back(); stack.push_back(b == 0.0 ? 0.0 : a / b); }
        }
        else if (op == "abs") { if (!stack.empty()) stack.back() = std::abs(stack.back()); }
        else if (op == "1index" || op == "2index" || op == "3index" || op == "4index") {
            size_t n = (size_t)(op[0] - '0');
            if (stack.size() > n) stack.push_back(stack[stack.size() - 1 - n]);
        }
        else break; // unknown operator; stop safely
    }

    std::vector<double> result;
    size_t take = std::min((size_t)expectedOutputs, stack.size());
    for (size_t k = stack.size() - take; k < stack.size(); ++k) result.push_back(stack[k]);
    return result;
}

// ===================================================------- token helpers
struct Operand {
    PdfValue value;
    std::string raw;
    bool isOperator = false;
};

class ContentTokenizer {
public:
    explicit ContentTokenizer(const QByteArray& data) : m_parser(data) {}

    Operand next() {
        PdfToken t = m_parser.next();
        Operand op;
        if (t.type == PdfTokenType::End) { op.raw = "__END__"; return op; }
        if (t.type == PdfTokenType::Number) { op.value = PdfValue::makeNumber(t.number); op.raw = t.text; return op; }
        if (t.type == PdfTokenType::String) { op.value = PdfValue::makeString(t.str); op.raw = t.text; return op; }
        if (t.type == PdfTokenType::Name)   { op.value = PdfValue::makeName(QString::fromStdString(t.text)); op.raw = "/" + t.text; return op; }
        if (t.type == PdfTokenType::Bool)   { op.value = PdfValue::makeBool(t.boolean); op.raw = t.text; return op; }
        if (t.type == PdfTokenType::Null)   { op.value = PdfValue::makeNull(); op.raw = "null"; return op; }
        if (t.type == PdfTokenType::ArrayOpen) { op.value = m_parser.parseArray(); op.raw = "[..]"; return op; }
        if (t.type == PdfTokenType::DictOpen)  { op.value = m_parser.parseDictionary(); op.raw = "<<..>>"; return op; }
        op.isOperator = true;
        op.raw = t.text;
        return op;
    }

private:
    PdfTokenParser m_parser;
};

static void addUnsupported(ParseResult& out, const QString& note) {
    out.unsupported.push_back(note);
}

// ============================================================ state context
struct Ctx {
    QTransform ctm;
    double lineWidth = 1.0;
    Qt::PenCapStyle cap = Qt::FlatCap;
    Qt::PenJoinStyle join = Qt::MiterJoin;
    double miterLimit = 10.0;
    std::vector<float> dash;
    float dashPhase = 0.0f;
    Color fillColor;
    Color strokeColor;
    float fillAlpha = 1.0f;
    float strokeAlpha = 1.0f;
    std::vector<QTransform> stack;

    std::vector<ParsedPath::Element> curPath;
    std::vector<ClippedArea> clips;

    bool inText = false;
    QTransform textMatrix;
    QTransform textLeading;
    double leading = 0.0;
    double fontSize = 12.0;
    QString fontName;
    Color textFill;

    // current resolved color spaces (Arrays like [/Separation ...])
    PdfValue fillCS, strokeCS;
};

static Color namedColorFromCS(const PdfValue& cs, const std::vector<PdfValue>& comps, const PdfDocument& doc) {
    Color c;
    if (!cs.isArray() || cs.array().empty()) {
        // Fallback based on component count
        size_t n = comps.size();
        if (n == 1) { c.components = {(float)comps[0].number()}; c.type = Color::Type::Gray; }
        else if (n == 3) { c.components = {(float)comps[0].number(), (float)comps[1].number(), (float)comps[2].number()}; c.type = Color::Type::RGB; }
        else if (n == 4) { c.components = {(float)comps[0].number(), (float)comps[1].number(), (float)comps[2].number(), (float)comps[3].number()}; c.type = Color::Type::CMYK; }
        return c;
    }
    auto arr = cs.array();
    QString csType = arr[0].isName() ? arr[0].nameValue() : QString();

    if (csType == "Separation" && arr.size() >= 4) {
        if (arr[1].isName()) {
            c.type = Color::Type::Spot;
            c.spotName = arr[1].nameValue();
            c.tint = comps.empty() ? 1.0f : (float)comps[0].number();
            // Evaluate tint transform (FunctionType 4 usually)
            double tint = comps.empty() ? 1.0 : comps[0].number();
            PdfValue func = doc.resolve(arr[3]);
            if (func.isDictionary()) {
                PdfValue typeV = func.dictGet("FunctionType");
                int ftype = typeV.isNumber() ? (int)typeV.number() : 4;
                if (ftype == 4) {
                    // Find the function object's stream by re-scanning for its dict
                    // (ref stored in arr[3] handled by doc.resolve? resolve returns the dict value only)
                    // We need stream bytes; doc.resolve would need doc internals.
                    // Workaround: search objects for dict with /FunctionType 4 matching this ref is not available here.
                    // Instead: caller passes resolved resources with stream merged (see import).
                    // For now, approximate with base CMYK func if present as /C0/C1/N (ftype 2)
                    if (func.dictHas("C0") && func.dictHas("C1") && func.dictHas("N") && ftype == 2) {
                        auto c0 = func.getArray("C0");
                        auto c1 = func.getArray("C1");
                        double n = func.getNumber("N", 1.0);
                        std::vector<double> outc(4, 0.0);
                        for (int i = 0; i < 4 && i < (int)c0.size(); ++i) {
                            double a = c0[i].isNumber() ? c0[i].number() : 0.0;
                            double b = c1[i].isNumber() ? c1[i].number() : 0.0;
                            outc[i] = a + (b - a) * std::pow(tint, n);
                        }
                        c.components = {(float)outc[0], (float)outc[1], (float)outc[2], (float)outc[3]};
                    }
                }
            }
            return c;
        }
    }

    if (csType == "DeviceN" && arr.size() >= 2 && arr[1].isArray()) {
        auto names = arr[1].array();
        if (!names.empty() && names[0].isName()) {
            c.type = Color::Type::Spot;
            c.spotName = names[0].nameValue();
            c.tint = comps.empty() ? 1.0f : (float)comps[0].number();
            return c;
        }
    }

    // Device spaces
    if (comps.size() == 4) { c.components = {(float)comps[0].number(), (float)comps[1].number(), (float)comps[2].number(), (float)comps[3].number()}; c.type = Color::Type::CMYK; }
    else if (comps.size() == 3) { c.components = {(float)comps[0].number(), (float)comps[1].number(), (float)comps[2].number()}; c.type = Color::Type::RGB; }
    else if (comps.size() == 1) { c.components = {(float)comps[0].number()}; c.type = Color::Type::Gray; }
    return c;
}

// ============================================================ main entry
ParseResult PdfContentParser::parse(const QByteArray& content,
                                    const PdfDocument& doc,
                                    const PdfValue& resources,
                                    const QTransform& initialCtm,
                                    double pageHeight) {
    ParseResult out;
    Ctx ctx;
    ctx.ctm = QTransform(1, 0, 0, -1, 0, pageHeight) * initialCtm;
    ctx.fillColor = Color::rgb(0, 0, 0);
    ctx.strokeColor = Color::rgb(0, 0, 0);
    ctx.textFill = Color::rgb(0, 0, 0);

    ContentTokenizer tok(content);

    // Build resolved name->dict maps for the page resources
    std::map<std::string, PdfValue> colorSpaces;
    std::map<std::string, PdfValue> extGStates;
    if (resources.isDictionary()) {
        PdfValue cs = resources.dictGet("ColorSpace");
        PdfValue gs = resources.dictGet("ExtGState");
        auto resolveDictFromPossibleRef = [&](const PdfValue& v) -> PdfValue {
            if (v.isRef()) return doc.resolve(v);
            return v;
        };
        PdfValue csDict = resolveDictFromPossibleRef(cs);
        if (csDict.isDictionary()) {
            for (const auto& [k, v] : csDict.dict()) {
                colorSpaces[k] = resolveDictFromPossibleRef(v);
            }
        }
        PdfValue gsDict = resolveDictFromPossibleRef(gs);
        if (gsDict.isDictionary()) {
            for (const auto& [k, v] : gsDict.dict()) {
                extGStates[k] = resolveDictFromPossibleRef(v);
            }
        }
    }

    auto tf = [&](double x, double y) -> QPointF {
        return ctx.ctm.map(QPointF(x, y));
    };

    auto flushPath = [&](bool fill, bool stroke, bool evenOdd, bool close) {
        if (ctx.curPath.empty()) return;
        ParsedPath p;
        p.elements = ctx.curPath;
        if (close) p.elements.push_back({ParsedPath::Element::T::Close, {}, {}, {}});
        p.hasFill = fill;
        p.hasStroke = stroke;
        p.fillColor = ctx.fillColor;
        p.strokeColor = ctx.strokeColor;
        p.lineWidth = (float)ctx.lineWidth;
        p.capStyle = ctx.cap;
        p.joinStyle = ctx.join;
        p.miterLimit = ctx.miterLimit;
        p.dashPattern = ctx.dash;
        p.dashPhase = ctx.dashPhase;
        p.fillAlpha = ctx.fillAlpha;
        p.strokeAlpha = ctx.strokeAlpha;
        p.evenOddFill = evenOdd;
        out.paths.push_back(p);
        ctx.curPath.clear();
    };

    auto currentPoint = [&]() -> QPointF {
        if (ctx.curPath.empty()) return QPointF();
        const auto& e = ctx.curPath.back();
        if (e.t == ParsedPath::Element::T::Cubic) return e.p3;
        return e.p1;
    };

    std::vector<PdfValue> operands;

    while (true) {
        Operand op = tok.next();
        if (op.raw == "__END__") break;
        if (!op.isOperator) { operands.push_back(op.value); continue; }

        const std::string& o = op.raw;

        // -------- graphics state
        if (o == "q") { ctx.stack.push_back(ctx.ctm); operands.clear(); continue; }
        if (o == "Q") { if (!ctx.stack.empty()) { ctx.ctm = ctx.stack.back(); ctx.stack.pop_back(); } operands.clear(); continue; }
        if (o == "cm" && operands.size() >= 6) {
            size_t b = operands.size() - 6;
            double a = operands[b].number(), bb = operands[b+1].number(),
                   c = operands[b+2].number(), d = operands[b+3].number(),
                   e = operands[b+4].number(), f = operands[b+5].number();
            ctx.ctm = ctx.ctm * QTransform(a, bb, c, d, e, f);
            operands.clear(); continue;
        }
        if (o == "w") { if (!operands.empty()) ctx.lineWidth = operands.back().number(); operands.clear(); continue; }
        if (o == "J") { if (!operands.empty()) { int v = (int)operands.back().number(); ctx.cap = v == 0 ? Qt::FlatCap : v == 1 ? Qt::RoundCap : Qt::SquareCap; } operands.clear(); continue; }
        if (o == "j") { if (!operands.empty()) { int v = (int)operands.back().number(); ctx.join = v == 0 ? Qt::MiterJoin : v == 1 ? Qt::RoundJoin : Qt::BevelJoin; } operands.clear(); continue; }
        if (o == "M") { if (!operands.empty()) ctx.miterLimit = operands.back().number(); operands.clear(); continue; }
        if (o == "d" && operands.size() >= 2) {
            PdfValue arr = operands[operands.size()-2];
            ctx.dash.clear();
            for (const auto& v : arr.array()) ctx.dash.push_back((float)v.number());
            ctx.dashPhase = (float)operands.back().number();
            operands.clear(); continue;
        }

        // -------- colors
        if (o == "g" || o == "G") {
            if (!operands.empty()) {
                Color c; c.components = {(float)operands.back().number()}; c.type = Color::Type::Gray;
                if (o == "g") ctx.fillColor = c; else ctx.strokeColor = c;
            }
            operands.clear(); continue;
        }
        if (o == "rg" || o == "RG") {
            if (operands.size() >= 3) {
                size_t b = operands.size() - 3;
                Color c; c.components = {(float)operands[b].number(), (float)operands[b+1].number(), (float)operands[b+2].number()}; c.type = Color::Type::RGB;
                if (o == "rg") ctx.fillColor = c; else ctx.strokeColor = c;
            }
            operands.clear(); continue;
        }
        if (o == "k" || o == "K") {
            if (operands.size() >= 4) {
                size_t b = operands.size() - 4;
                Color c; c.components = {(float)operands[b].number(), (float)operands[b+1].number(), (float)operands[b+2].number(), (float)operands[b+3].number()}; c.type = Color::Type::CMYK;
                if (o == "k") ctx.fillColor = c; else ctx.strokeColor = c;
            }
            operands.clear(); continue;
        }

        // cs/CS: set color space
        if (o == "cs" || o == "CS") {
            PdfValue nameOp = operands.empty() ? PdfValue::makeNull() : operands.back();
            std::string nm;
            if (nameOp.isName()) nm = nameOp.nameValue().toStdString();
            auto it = colorSpaces.find(nm);
            PdfValue resolved = (it != colorSpaces.end()) ? it->second : PdfValue::makeNull();
            if (o == "cs") ctx.fillCS = resolved; else ctx.strokeCS = resolved;
            operands.clear(); continue;
        }
        // sc/SC/scn/SCN: set color components
        if (o == "sc" || o == "SC" || o == "scn" || o == "SCN") {
            bool fill = (o == "sc" || o == "scn");
            PdfValue cs = fill ? ctx.fillCS : ctx.strokeCS;
            Color c = namedColorFromCS(cs, operands, doc);
            if (fill) ctx.fillColor = c; else ctx.strokeColor = c;
            operands.clear(); continue;
        }

        // -------- path construction
        if (o == "m" && operands.size() >= 2) {
            size_t b = operands.size() - 2;
            ctx.curPath.push_back({ParsedPath::Element::T::Move, tf(operands[b].number(), operands[b+1].number()), {}, {}});
            operands.clear(); continue;
        }
        if (o == "l" && operands.size() >= 2) {
            size_t b = operands.size() - 2;
            ctx.curPath.push_back({ParsedPath::Element::T::Line, tf(operands[b].number(), operands[b+1].number()), {}, {}});
            operands.clear(); continue;
        }
        if (o == "c" && operands.size() >= 6) {
            size_t b = operands.size() - 6;
            ctx.curPath.push_back({ParsedPath::Element::T::Cubic,
                                   tf(operands[b+0].number(), operands[b+1].number()),
                                   tf(operands[b+2].number(), operands[b+3].number()),
                                   tf(operands[b+4].number(), operands[b+5].number())});
            operands.clear(); continue;
        }
        if (o == "v" && operands.size() >= 4) {
            size_t b = operands.size() - 4;
            ctx.curPath.push_back({ParsedPath::Element::T::Cubic,
                                   currentPoint(),
                                   tf(operands[b+0].number(), operands[b+1].number()),
                                   tf(operands[b+2].number(), operands[b+3].number())});
            operands.clear(); continue;
        }
        if (o == "y" && operands.size() >= 4) {
            size_t b = operands.size() - 4;
            QPointF c2 = tf(operands[b+2].number(), operands[b+3].number());
            ctx.curPath.push_back({ParsedPath::Element::T::Cubic,
                                   tf(operands[b+0].number(), operands[b+1].number()),
                                   c2,
                                   c2});
            operands.clear(); continue;
        }
        if (o == "re" && operands.size() >= 4) {
            size_t b = operands.size() - 4;
            double x = operands[b].number(), y = operands[b+1].number();
            double w = operands[b+2].number(), h = operands[b+3].number();
            ctx.curPath.push_back({ParsedPath::Element::T::Move, tf(x, y + h), {}, {}});
            ctx.curPath.push_back({ParsedPath::Element::T::Line, tf(x + w, y + h), {}, {}});
            ctx.curPath.push_back({ParsedPath::Element::T::Line, tf(x + w, y), {}, {}});
            ctx.curPath.push_back({ParsedPath::Element::T::Line, tf(x, y), {}, {}});
            ctx.curPath.push_back({ParsedPath::Element::T::Close, {}, {}, {}});
            operands.clear(); continue;
        }
        if (o == "h") { ctx.curPath.push_back({ParsedPath::Element::T::Close, {}, {}, {}}); operands.clear(); continue; }

        // -------- painting
        if (o == "S") { flushPath(false, true, false, false); operands.clear(); continue; }
        if (o == "s") { ctx.curPath.push_back({ParsedPath::Element::T::Close, {}, {}, {}}); flushPath(false, true, false, false); operands.clear(); continue; }
        if (o == "f" || o == "F") { flushPath(true, false, false, false); operands.clear(); continue; }
        if (o == "f*") { flushPath(true, false, true, false); operands.clear(); continue; }
        if (o == "B") { flushPath(true, true, false, false); operands.clear(); continue; }
        if (o == "B*") { flushPath(true, true, true, false); operands.clear(); continue; }
        if (o == "b") { ctx.curPath.push_back({ParsedPath::Element::T::Close, {}, {}, {}}); flushPath(true, true, false, false); operands.clear(); continue; }
        if (o == "b*") { ctx.curPath.push_back({ParsedPath::Element::T::Close, {}, {}, {}}); flushPath(true, true, true, false); operands.clear(); continue; }
        if (o == "n") { ctx.curPath.clear(); operands.clear(); continue; }

        // -------- clipping
        if (o == "W" || o == "W*") {
            if (!ctx.curPath.empty()) {
                ClippedArea area;
                area.elements = ctx.curPath;
                area.evenOdd = (o == "W*");
                out.clips.push_back(area);
            }
            operands.clear(); continue;
        }

        // -------- text objects
        if (o == "BT") { ctx.inText = true; ctx.textMatrix = QTransform(); ctx.textLeading = QTransform(); operands.clear(); continue; }
        if (o == "ET") { ctx.inText = false; operands.clear(); continue; }
        if (o == "Tf") {
            if (operands.size() >= 2) {
                PdfValue fontOp = operands[operands.size()-2];
                ctx.fontName = fontOp.isName() ? fontOp.nameValue() : QString("Helvetica");
                ctx.fontSize = operands.back().number();
            }
            operands.clear(); continue;
        }
        if (o == "TL") { if (!operands.empty()) ctx.leading = operands.back().number(); operands.clear(); continue; }
        if (o == "Td" || o == "TD") {
            if (operands.size() >= 2) {
                size_t b = operands.size() - 2;
                double tx = operands[b].number(), ty = operands[b+1].number();
                if (o == "TD") ctx.leading = -ty;
                QTransform shift(1, 0, 0, 1, tx, ty);
                ctx.textMatrix = shift * ctx.textMatrix;
                ctx.textLeading = ctx.textMatrix;
            }
            operands.clear(); continue;
        }
        if (o == "Tm") {
            if (operands.size() >= 6) {
                size_t b = operands.size() - 6;
                double a = operands[b].number(), bb = operands[b+1].number(),
                       c = operands[b+2].number(), d = operands[b+3].number(),
                       e = operands[b+4].number(), f = operands[b+5].number();
                ctx.textMatrix = QTransform(a, bb, c, d, e, f);
                ctx.textLeading = ctx.textMatrix;
            }
            operands.clear(); continue;
        }
        if (o == "T*") {
            QTransform shift(1, 0, 0, 1, 0, ctx.leading == 0.0 ? -ctx.fontSize : -ctx.leading);
            ctx.textMatrix = shift * ctx.textMatrix;
            ctx.textLeading = ctx.textMatrix;
            operands.clear(); continue;
        }
        if (o == "Tj" || o == "TJ" || o == "'" || o == "\"") {
            QString text;
            for (const auto& v : operands) {
                if (v.isString()) text += QString::fromLatin1(v.stringValue());
                else if (v.isArray()) {
                    for (const auto& av : v.array()) if (av.isString()) text += QString::fromLatin1(av.stringValue());
                }
            }
            if (o == "'" || o == "\"") {
                // move to next line and show
                QTransform shift(1, 0, 0, 1, 0, ctx.leading == 0.0 ? -ctx.fontSize : -ctx.leading);
                ctx.textMatrix = shift * ctx.textMatrix;
                ctx.textLeading = ctx.textMatrix;
            }
            if (!text.isEmpty()) {
                ParsedText t;
                t.text = text;
                t.fontName = ctx.fontName;
                t.fontSize = (float)ctx.fontSize;
                t.fillColor = ctx.fillColor;
                t.transform = ctx.ctm * ctx.textMatrix;
                t.fillAlpha = ctx.fillAlpha;
                out.texts.push_back(t);
            }
            operands.clear(); continue;
        }

        // -------- XObjects
        if (o == "Do") {
            if (!operands.empty()) {
                PdfValue nameOp = operands.back();
                if (nameOp.isName()) {
                    ParsedImage img;
                    img.resourceName = nameOp.nameValue();
                    img.transform = ctx.ctm;
                    img.displayWidth = std::abs(ctx.ctm.m11());
                    img.displayHeight = std::abs(ctx.ctm.m22());
                    out.images.push_back(img);
                }
            }
            operands.clear(); continue;
        }

        // -------- ExtGState
        if (o == "gs") {
            if (!operands.empty()) {
                PdfValue nameOp = operands.back();
                if (nameOp.isName()) {
                    std::string nm = nameOp.nameValue().toStdString();
                    auto it = extGStates.find(nm);
                    if (it != extGStates.end()) {
                        PdfValue gsObj = it->second;
                        if (gsObj.isDictionary()) {
                            if (gsObj.dictHas("ca")) ctx.fillAlpha = (float)gsObj.getNumber("ca", 1.0);
                            if (gsObj.dictHas("CA")) ctx.strokeAlpha = (float)gsObj.getNumber("CA", 1.0);
                        }
                    } else {
                        addUnsupported(out, QString("ExtGState %1 not found in resources").arg(QString::fromStdString(nm)));
                    }
                }
            }
            operands.clear(); continue;
        }

        // -------- known-but-untracked operators
        if (o == "BMC" || o == "EMC" || o == "BDC" || o == "ri" || o == "i" || o == "MP" || o == "DP" || o == "BX" || o == "EX" || o == "sh" || o == "d0" || o == "d1" || o == "Tf" || o == "Tc" || o == "Tw" || o == "Tz" || o == "Ts" || o == "Tr" || o == "T*") {
            // Tc/Tw/Tz/Ts/Tr: text state we track only partially
            if (o == "Tr") { /* render mode: ignore */ }
            operands.clear(); continue;
        }

        addUnsupported(out, QString("Unsupported operator: %1").arg(QString::fromStdString(o)));
        operands.clear();
    }

    return out;
}

// Helper to read a FunctionType 2/3/4 stream for Separation evaluation.
// Kept separate from the tokenizer loop.
} // namespace PDF
} // namespace ArtPlus
