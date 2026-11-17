#include "pdf_exporter.h"
#include <QtZlib/zlib.h>
#include <QFile>
#include <QImage>
#include <QBuffer>
#include <sstream>
#include <iomanip>

namespace ArtPlus {
namespace PDF {

// =====================================================================
// Object record: text header + optional binary stream payload
// =====================================================================
struct ObjectRecord {
    std::string header;            // "<< /Type /Page ... >>" or full "<< /Length N >>\nstream\n"
    std::vector<uint8_t> stream;   // optional binary stream payload (empty for non-stream)
    bool isStream = false;
    std::string footer;            // usually empty; "endstream" added by writer for streams
};

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

// Lossless RGB image prep for /Subtype /Image /Filter /FlateDecode
PdfExporter::ImageEmbedding PdfExporter::prepareImage(const QImage& img) {
    ImageEmbedding out;
    out.objectNum = 0;
    QImage rgb = img.convertToFormat(QImage::Format_RGB888);
    out.width = rgb.width();
    out.height = rgb.height();
    const uchar* bits = rgb.constBits();
    out.rgbData.assign(bits, bits + (size_t)rgb.sizeInBytes());
    return out;
}

std::vector<uint8_t> PdfExporter::flateCompress(const std::vector<uint8_t>& raw) {
    uLongf bound = compressBound((uLong)raw.size()) + 32;
    std::vector<uint8_t> out(bound);
    if (compress2(out.data(), &bound, raw.data(), (uLong)raw.size(), 9) != Z_OK) {
        out.clear();
        return out;
    }
    out.resize(bound);
    return out;
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

    // Object layout (1-indexed):
    //   1 = Catalog, 2 = Pages, 3..(2+numPages) = Page dicts
    //   (3+numPages)..(2+2*numPages) = Contents
    //   (3+2*numPages) = Font
    //   (4+2*numPages).. = Image XObjects (assigned per image object in doc order)
    const int contentBase = 3 + numPages;
    const int fontNum = contentBase + numPages;
    const int imageBase = fontNum + 1;

    // Precompute boxes, draw orders, and image assignments (deterministic, per file order)
    std::vector<QRectF> boxes;
    std::vector<std::vector<std::shared_ptr<DocumentObject>>> orders;
    std::vector<std::vector<int>> pageImageNums;   // per page: list of XObject object numbers
    std::vector<std::vector<ImageEmbedding>> pageImageData; // per page: parallel embeddings
    std::map<ObjectId, int> imageXObjectNum;
    int nextImageNum = imageBase;

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

        std::vector<int> xnums;
        std::vector<ImageEmbedding> embeds;
        for (const auto& obj : orders.back()) {
            if (obj->type() == ObjectType::Image) {
                auto imgObj = std::dynamic_pointer_cast<ImageObject>(obj);
                if (!imgObj) continue;
                int xobj;
                auto it = imageXObjectNum.find(obj->id());
                if (it != imageXObjectNum.end()) xobj = it->second;
                else { xobj = nextImageNum++; imageXObjectNum[obj->id()] = xobj; }
                ImageEmbedding emb = prepareImage(imgObj->image());
                emb.objectNum = xobj;
                embeds.push_back(std::move(emb));
                xnums.push_back(xobj);
            }
        }
        pageImageNums.push_back(std::move(xnums));
        pageImageData.push_back(std::move(embeds));
    }

    // Prepare image embedding lookup by object number
    std::map<int, ImageEmbedding> imagesByNum;
    for (const auto& embeds : pageImageData) for (const auto& emb : embeds) imagesByNum[emb.objectNum] = emb;

    // Build content streams with image Do operators
    std::vector<std::string> contents;
    for (size_t pi = 0; pi < orders.size(); ++pi) {
        std::ostringstream content;
        size_t imgIdx = 0;
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
            else if (obj->type() == ObjectType::Image) {
                int xobj = pageImageNums[pi][imgIdx++];
                content << "/Im" << xobj << " Do\n";
            }
            content << "Q\n";
        }
        contents.push_back(content.str());
    }

    // Build object records
    std::vector<ObjectRecord> records;
    auto add = [&](const std::string& body) -> int {
        ObjectRecord r;
        r.header = body;
        records.push_back(std::move(r));
        return (int)records.size();
    };
    auto addStream = [&](const std::string& dictHeader, const std::vector<uint8_t>& streamData) -> int {
        ObjectRecord r;
        r.header = dictHeader;
        r.stream = streamData;
        r.isStream = true;
        records.push_back(std::move(r));
        return (int)records.size();
    };

    add("<< /Type /Catalog /Pages 2 0 R >>");
    {
        std::ostringstream o;
        o << "<< /Type /Pages /Kids [";
        for (int i = 0; i < numPages; ++i) o << " " << (3 + i) << " 0 R";
        o << " ] /Count " << numPages << " >>";
        add(o.str());
    }
    for (int i = 0; i < numPages; ++i) {
        int contentsNum = contentBase + i;
        std::ostringstream o;
        o << "<< /Type /Page /Parent 2 0 R /MediaBox [ 0 0 " << (int)boxes[i].width() << " " << (int)boxes[i].height()
          << " ] /Contents " << contentsNum << " 0 R /Resources << /Font << /F1 " << fontNum << " 0 R >>";
        if (!pageImageNums[i].empty()) {
            o << " /XObject <<";
            for (int xobj : pageImageNums[i]) o << " /Im" << xobj << " " << xobj << " 0 R";
            o << " >>";
        }
        o << " >> >>";
        add(o.str());
    }
    for (int i = 0; i < numPages; ++i) {
        std::ostringstream o;
        o << "<< /Length " << contents[i].size() << " >>\nstream\n";
        std::vector<uint8_t> payload(contents[i].begin(), contents[i].end());
        addStream(o.str(), std::move(payload));
    }
    add("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>");

    // Image XObjects in fixed order (imageBase .. nextImageNum-1)
    for (int x = imageBase; x < nextImageNum; ++x) {
        auto it = imagesByNum.find(x);
        if (it == imagesByNum.end()) {
            // 1x1 white fallback (should not happen; keeps file valid)
            std::vector<uint8_t> white = flateCompress(std::vector<uint8_t>{0xFF, 0xFF, 0xFF});
            std::ostringstream o;
            o << "<< /Type /XObject /Subtype /Image /Width 1 /Height 1 /BitsPerComponent 8 /ColorSpace /DeviceRGB /Filter /FlateDecode /Length " << white.size() << " >>\nstream\n";
            addStream(o.str(), std::move(white));
            continue;
        }
        const ImageEmbedding& emb = it->second;
        std::vector<uint8_t> compressed = flateCompress(emb.rgbData);
        if (compressed.empty()) return false;
        std::ostringstream o;
        o << "<< /Type /XObject /Subtype /Image /Width " << emb.width << " /Height " << emb.height
          << " /BitsPerComponent 8 /ColorSpace /DeviceRGB /Filter /FlateDecode /Length " << compressed.size()
          << " >>\nstream\n";
        addStream(o.str(), std::move(compressed));
    }

    // Assemble the file
    std::vector<int64_t> offsets(records.size());
    QByteArray out;
    out += "%PDF-1.4\n";
    for (size_t i = 0; i < records.size(); ++i) {
        offsets[i] = out.size();
        out += std::to_string(i + 1).c_str();
        out += " 0 obj\n";
        out += records[i].header.c_str();
        if (records[i].isStream) {
            out += QByteArray((const char*)records[i].stream.data(), (int)records[i].stream.size());
            out += "\nendstream\n";
        } else {
            out += "\n";
        }
        out += "endobj\n";
    }
    int64_t xref = out.size();
    out += "xref\n0 ";
    out += std::to_string(records.size() + 1).c_str();
    out += "\n0000000000 65535 f \n";
    for (size_t i = 0; i < offsets.size(); ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%010lld 00000 n \n", (long long)offsets[i]);
        out += buf;
    }
    out += "trailer\n<< /Size ";
    out += std::to_string(records.size() + 1).c_str();
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
