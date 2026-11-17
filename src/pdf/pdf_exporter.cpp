#include "pdf_exporter.h"
#include <QFile>
#include <QImage>
#include <QBuffer>
#include <sstream>
#include <iomanip>

namespace ArtPlus {
namespace PDF {

// =====================================================================
// Helpers
// =====================================================================

QByteArray PdfExporter::escapeTextString(const QString& text) {
    QByteArray out;
    for (QChar ch : text) {
        ushort u = ch.unicode();
        char c = (u < 256) ? (char)u : (char)'?';
        if (c == '\\' || c == '(' || c == ')') {
            out += '\\';
            out += c;
        } else if (c >= 32 && c < 127) {
            out += c;
        } else {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\%03o", (unsigned char)c);
            out += buf;
        }
    }
    return out;
}

QString PdfExporter::pdfDateString(const QString& iso) {
    QString digits;
    for (QChar ch : iso) {
        if (ch.isDigit()) digits += ch;
        if (digits.size() >= 14) break;
    }
    if (digits.size() < 12) digits = QStringLiteral("20261117192421");
    return QStringLiteral("D:") + digits.left(14);
}

QByteArray PdfExporter::colorForOperator(const Color& c, bool stroke) {
    auto n = [&](double v) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.4f", v);
        QString s = QString::fromLatin1(buf);
        s.replace(QStringLiteral(","), QStringLiteral("."));
        return s.toLatin1();
    };

    if (c.type == Color::Type::Spot) {
        QByteArray name;
        for (QChar ch : c.spotName) {
            char ch8 = (ch.unicode() < 256) ? (char)ch.unicode() : '?';
            if (ch8 == '#') name += "#23";
            else if (ch8 == ' ') name += "#20";
            else name += ch8;
        }
        std::ostringstream oss;
        oss << "/" << name.constData() << " cs " << n(c.tint).constData() << " scn";
        return QByteArray::fromStdString(oss.str());
    }

    if (c.type == Color::Type::CMYK && c.components.size() >= 4) {
        std::ostringstream oss;
        oss << n(c.components[0]).constData() << " " << n(c.components[1]).constData() << " " << n(c.components[2]).constData() << " " << n(c.components[3]).constData()
            << (stroke ? " K" : " k");
        return QByteArray::fromStdString(oss.str());
    }
    if (c.type == Color::Type::RGB && c.components.size() >= 3) {
        std::ostringstream oss;
        oss << n(c.components[0]).constData() << " " << n(c.components[1]).constData() << " " << n(c.components[2]).constData()
            << (stroke ? " RG" : " rg");
        return QByteArray::fromStdString(oss.str());
    }
    if (c.type == Color::Type::Gray && !c.components.isEmpty()) {
        std::ostringstream oss;
        oss << n(c.components[0]).constData() << (stroke ? " G" : " g");
        return QByteArray::fromStdString(oss.str());
    }
    return stroke ? QByteArray("0 0 0 RG") : QByteArray("0 0 0 rg");
}

// =====================================================================
// Export
// =====================================================================

bool PdfExporter::exportPdf(const Document& doc, const QString& outPath,
                            const QString& originalImportedPath) {
    // Faithful passthrough for unmodified imported documents
    if (!doc.modified() && !originalImportedPath.isEmpty()) {
        QFile src(originalImportedPath);
        if (src.open(QIODevice::ReadOnly)) {
            QFile dst(outPath);
            if (!dst.open(QIODevice::WriteOnly)) return false;
            dst.write(src.readAll());
            return true;
        }
    }

    const int numPages = (int)doc.pageOrder().size();

    // XML object numbers:
    //   1 = Catalog, 2 = Pages, 3..(2+numPages) = Page dicts
    //   (3+numPages)..(2+2*numPages) = Contents, (3+2*numPages) = Font
    const int contentBase = 3 + numPages;
    const int fontNum = 3 + 2 * numPages;

    // Precompute boxes and draw orders
    std::vector<QRectF> boxes;
    std::vector<std::vector<std::shared_ptr<DocumentObject>>> orders;
    for (PageId pid : doc.pageOrder()) {
        auto page = doc.page(pid);
        boxes.push_back(page->boxes().mediaBox);
        std::vector<std::shared_ptr<DocumentObject>> order;
        for (LayerId lid : doc.layerOrder()) {
            auto layer = doc.layer(lid);
            if (!layer) continue;
            bool belongs = std::find(page->layers().begin(), page->layers().end(), lid) != page->layers().end();
            for (ObjectId oid : layer->objects()) {
                auto obj = doc.object(oid);
                if (obj && belongs) order.push_back(obj);
            }
        }
        if (order.empty()) for (const auto& [id, obj] : doc.objects()) order.push_back(obj);
        orders.push_back(std::move(order));
    }

    // Generate contents
    // Note: imported coordinates are already in Qt (y-down) space; export them as-is.
    std::vector<std::string> contents;
    for (size_t pi = 0; pi < orders.size(); ++pi) {
        std::ostringstream content;
        for (const auto& obj : orders[pi]) {
            content << "q\n";
            QTransform t = obj->transform();
            content << t.m11() << " " << t.m12() << " " << t.m21() << " " << t.m22() << " " << t.m31() << " " << t.m32() << " cm\n";

            if (obj->type() == ObjectType::Path) {
                auto path = std::dynamic_pointer_cast<PathObject>(obj);
                for (const auto& e : path->elements()) {
                    switch (e.type) {
                        case PathObject::PathElement::Type::Move:
                            content << e.p1.x() << " " << e.p1.y() << " m\n"; break;
                        case PathObject::PathElement::Type::Line:
                            content << e.p1.x() << " " << e.p1.y() << " l\n"; break;
                        case PathObject::PathElement::Type::Curve:
                            content << e.p1.x() << " " << e.p1.y() << " "
                                    << e.p2.x() << " " << e.p2.y() << " "
                                    << e.p3.x() << " " << e.p3.y() << " c\n"; break;
                        case PathObject::PathElement::Type::Close:
                            content << "h\n"; break;
                    }
                }
                Color fill = obj->fillStroke().fill;
                Color stroke = obj->fillStroke().stroke;
                content << colorForOperator(fill, false).toStdString() << "\n";
                content << colorForOperator(stroke, true).toStdString() << "\n";
                content << obj->fillStroke().strokeWidth << " w\n";
                bool hasFill = fill.isValid();
                bool hasStroke = stroke.isValid();
                if (hasFill && hasStroke) content << "B\n";
                else if (hasFill) content << "f\n";
                else if (hasStroke) content << "S\n";
                else content << "n\n";
            }
            else if (obj->type() == ObjectType::Text) {
                auto text = std::dynamic_pointer_cast<TextObject>(obj);
                content << colorForOperator(obj->fillStroke().fill, false).toStdString() << "\n";
                content << "BT\n/F1 " << text->fontSize() << " Tf\n";
                content << "(" << escapeTextString(text->text()).toStdString() << ") Tj\nET\n";
            }
            content << "Q\n";
        }
        contents.push_back(content.str());
    }

    // Build object bodies (1-indexed)
    std::vector<std::string> bodies;
    bodies.push_back("<< /Type /Catalog /Pages 2 0 R >>");
    {
        std::ostringstream o;
        o << "<< /Type /Pages /Kids [";
        for (int i = 0; i < numPages; ++i) o << " " << (3 + i) << " 0 R";
        o << " ] /Count " << numPages << " >>";
        bodies.push_back(o.str());
    }
    for (int i = 0; i < numPages; ++i) {
        int contentsNum = contentBase + i;
        std::ostringstream o;
        o << "<< /Type /Page /Parent 2 0 R /MediaBox [ 0 0 " << (int)boxes[i].width() << " " << (int)boxes[i].height()
          << " ] /Contents " << contentsNum << " 0 R /Resources << /Font << /F1 " << fontNum << " 0 R >> >> >>";
        bodies.push_back(o.str());
    }
    for (int i = 0; i < numPages; ++i) {
        std::ostringstream o;
        o << "<< /Length " << contents[i].size() << " >>\nstream\n" << contents[i] << "endstream";
        bodies.push_back(o.str());
    }
    bodies.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>");

    // Compute offsets, write file
    std::vector<int64_t> offsets(bodies.size());
    QByteArray out;
    out += "%PDF-1.4\n";
    for (size_t i = 0; i < bodies.size(); ++i) {
        offsets[i] = out.size();
        out += std::to_string(i + 1).c_str();
        out += " 0 obj\n";
        out += bodies[i].c_str();
        out += "\nendobj\n";
    }
    int64_t xref = out.size();
    out += "xref\n0 ";
    out += std::to_string(bodies.size() + 1).c_str();
    out += "\n0000000000 65535 f \n";
    for (size_t i = 0; i < offsets.size(); ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%010lld 00000 n \n", (long long)offsets[i]);
        out += buf;
    }
    out += "trailer\n<< /Size ";
    out += std::to_string(bodies.size() + 1).c_str();
    out += " /Root 1 0 R >>\nstartxref\n";
    out += std::to_string(xref).c_str();
    out += "\n%%EOF";

    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(out);
    return true;
}

} // namespace PDF
} // namespace ArtPlus
