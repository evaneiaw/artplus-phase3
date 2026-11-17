#include "pdf_importer.h"
#include <algorithm>
#include <cmath>

namespace ArtPlus {
using namespace PDF;

QRectF PdfImporter::pathBounds(const std::vector<PDF::ParsedPath::Element>& elements) {
    QRectF rect;
    bool first = true;
    auto addPoint = [&](const QPointF& p) {
        if (first) { rect = QRectF(p, p); first = false; }
        else rect = rect.united(QRectF(p, p));
    };
    for (const auto& e : elements) {
        if (e.t == PDF::ParsedPath::Element::T::Close) continue;
        addPoint(e.p1);
        if (e.t == PDF::ParsedPath::Element::T::Cubic) {
            addPoint(e.p2);
            addPoint(e.p3);
        }
    }
    return rect;
}

Color PdfImporter::cmykToRgbColor(const Color& cmyk) {
    // Simple subtractive conversion used for display preview;
    // original CMYK components are kept in the object for export fidelity.
    float c = cmyk.components.size() > 0 ? cmyk.components[0] : 0;
    float m = cmyk.components.size() > 1 ? cmyk.components[1] : 0;
    float y = cmyk.components.size() > 2 ? cmyk.components[2] : 0;
    float k = cmyk.components.size() > 3 ? cmyk.components[3] : 0;
    Color rgb;
    rgb.type = Color::Type::RGB;
    rgb.components = { (1.0f - c) * (1.0f - k), (1.0f - m) * (1.0f - k), (1.0f - y) * (1.0f - k) };
    return rgb;
}

QImage PdfImporter::readImageXObject(const PDF::PdfDocument& doc, const PDF::ParsedImage& imgRef) {
    // The xObject reference is stored as object number/generation in ParsedImage.
    // For the initial version we locate the image via Resources of the page;
    // the caller passes xObjectNumber via resource resolution.
    (void)doc; (void)imgRef;
    // Placeholder: will be wired with the full resource resolution in the
    // importer path below (monolithic for simplicity).
    return QImage();
}

PdfImportResult PdfImporter::import(const PDF::PdfDocument& doc, Document& out) {
    PdfImportResult result;
    if (!doc.isOpen()) {
        result.error = "Document not open";
        return result;
    }

    const int nPages = doc.pageCount();
    if (nPages <= 0) {
        result.error = "PDF has no pages";
        return result;
    }

    for (int pageIndex = 0; pageIndex < nPages; ++pageIndex) {
        // Page geometry in PDF units, converted to Qt (y down)
        PDF::PageGeometry geo = doc.pageGeometry(pageIndex);
        double pageHeight = geo.height();
        double pageWidth = geo.width();
        double llx = geo.cropBox[0], lly = geo.cropBox[1];

        // Qt coordinates: y' = pageHeight - y
        QRectF mediaBox(0, 0, pageWidth, pageHeight);
        // content coordinates from parser are already y-down flipped using pageHeight
        // but with origin at PDF cropBox origin. For simplicity we treat cropBox as the page.
        // (If cropBox has non-zero origin, offset is applied via CTM translation by the caller.)
        QRectF cropBox(llx, pageHeight - (lly + geo.cropBox[3]), pageWidth, pageHeight);
        // Actually simpler: mediaBox/cropBox as y-flipped rect
        cropBox = QRectF(0, 0, pageWidth, pageHeight);

        auto page = std::make_shared<Page>();
        page->boxes().mediaBox = QRectF(0, 0, pageWidth, pageHeight);
        page->boxes().cropBox  = cropBox;
        page->boxes().bleedBox = QRectF(0, 0, pageWidth, pageHeight);
        page->boxes().trimBox  = QRectF(0, 0, pageWidth, pageHeight);
        page->boxes().artBox   = QRectF(0, 0, pageWidth, pageHeight);

        // One layer per page to keep hierarchy inspectable
        auto layer = std::make_shared<Layer>(QString("Page %1").arg(pageIndex + 1));
        out.addLayer(layer);
        page->addLayer(layer->id());
        out.addPage(page);

        // Parse content stream
        QByteArray contents = doc.pageContents(pageIndex);
        PDF::ParseResult parsed = PDF::PdfContentParser::parse(contents, doc, doc.pageResources(pageIndex), QTransform(), pageHeight);

        for (const auto& unsup : parsed.unsupported) {
            result.warnings.push_back(unsup);
        }

        // Convert paths
        for (const auto& p : parsed.paths) {
            auto obj = std::make_shared<PathObject>();
            obj->setLayerId(layer->id());
            for (const auto& e : p.elements) {
                switch (e.t) {
                    case PDF::ParsedPath::Element::T::Move:  obj->addMove(e.p1); break;
                    case PDF::ParsedPath::Element::T::Line:  obj->addLine(e.p1); break;
                    case PDF::ParsedPath::Element::T::Cubic: obj->addCurve(e.p1, e.p2, e.p3); break;
                    case PDF::ParsedPath::Element::T::Close: obj->closePath(); break;
                }
            }
            obj->fillStroke().strokeWidth = p.lineWidth;
            obj->fillStroke().capStyle = p.capStyle;
            obj->fillStroke().joinStyle = p.joinStyle;
            obj->fillStroke().miterLimit = (float)p.miterLimit;
            obj->fillStroke().dashPattern = p.dashPattern;
            obj->fillStroke().dashOffset = p.dashPhase;
            if (p.hasFill) obj->fillStroke().fill = p.fillColor;
            if (p.hasStroke) obj->fillStroke().stroke = p.strokeColor;
            obj->setName(QString("Path %1").arg(++result.pathsImported));
            out.addObject(obj);
            layer->addObject(obj->id());
            ++result.objectsImported;
        }

        // Convert texts
        for (const auto& t : parsed.texts) {
            auto obj = std::make_shared<TextObject>();
            obj->setLayerId(layer->id());
            obj->setText(t.text);
            // PDF base fonts: Helvetica/Times/Courier families
            QString fam = t.fontName;
            bool bold = fam.contains("Bold", Qt::CaseInsensitive);
            bool italic = fam.contains("Italic", Qt::CaseInsensitive) || fam.contains("Oblique", Qt::CaseInsensitive);
            fam.replace("-Bold", "").replace("-Italic", "").replace("-Oblique", "");
            if (fam.isEmpty()) fam = "Helvetica";
            obj->setFontFamily(fam);
            obj->setBold(bold);
            obj->setItalic(italic);
            obj->setFontSize(t.fontSize);
            obj->setTransform(t.transform);
            obj->fillStroke().fill = t.fillColor;
            obj->setName(QString("Text %1").arg(++result.textsImported));
            out.addObject(obj);
            layer->addObject(obj->id());
            ++result.objectsImported;
        }

        // Convert images (stub for XObject resolution; records as placeholder object)
        for (const auto& img : parsed.images) {
            auto obj = std::make_shared<ImageObject>();
            obj->setLayerId(layer->id());
            // Try to extract image data from the XObject dictionary
            // Locate resource ref from page Resources
            PdfValue res = doc.pageResources(pageIndex);
            PdfValue xobjs = res.dictGet("XObject");
            std::string key = img.resourceName.toStdString();
            if (!key.empty() && key[0] == '/') key = key.substr(1);
            PdfValue ref = xobjs.dictGet(key);
            if (ref.isRef()) {
                const PdfObject* xo = doc.findObject(ref.refObj(), ref.refGen());
                if (xo && xo->isStream) {
                    // Attempt to decode as JPEG (DCTDecode) or raw RGB (FlateDecode)
                    std::map<std::string, PdfValue> dict = xo->value.dict();
                    auto subtype = dict.find("Subtype");
                    bool isImage = subtype != dict.end() && subtype->second.isName() && subtype->second.nameValue() == "Image";
                    if (isImage) {
                        QByteArray raw((const char*)xo->stream.data(), (int)xo->stream.size());
                        QImage decoded(raw);
                        int w = xo->value.getInt("Width", 0);
                        int h = xo->value.getInt("Height", 0);
                        if (!decoded.isNull()) {
                            obj->setImage(decoded.scaled((int)img.displayWidth, (int)img.displayHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                        } else if (w > 0 && h > 0 && w * h * 3 <= raw.size()) {
                            // raw RGB FlateDecode data
                            QImage tmp(w, h, QImage::Format_RGB888);
                            memcpy(tmp.bits(), raw.constData(), (size_t)tmp.sizeInBytes());
                            obj->setImage(tmp);
                        }
                    }
                }
            }
            obj->setTransform(img.transform);
            obj->setName(QString("Image %1").arg(++result.imagesImported));
            if (obj->image().isNull()) {
                // Record as unsupported if we could not decode
                QImage placeholder((int)img.displayWidth, (int)img.displayHeight, QImage::Format_RGB32);
                placeholder.fill(Qt::darkGray);
                obj->setImage(placeholder);
                result.warnings.push_back(QString("Image '%1' could not be decoded; placeholder inserted").arg(img.resourceName));
            }
            out.addObject(obj);
            layer->addObject(obj->id());
            ++result.objectsImported;
        }

        // Clips are recorded as warnings (full clipping mask support is future work)
        for (const auto& clip : parsed.clips) {
            result.warnings.push_back(QString("Clipping path (%1 elements) noted on page %2").arg(clip.elements.size()).arg(pageIndex + 1));
        }

        ++result.pagesImported;
    }

    result.ok = true;
    return result;
}

} // namespace ArtPlus
