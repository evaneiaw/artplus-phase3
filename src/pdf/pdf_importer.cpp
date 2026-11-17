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

        // Convert XObjects (images vs forms recursively)
        for (const auto& xobjPlaceholder : parsed.images) {
            // Resolve XObject dict from page Resources
            PdfValue res = doc.pageResources(pageIndex);
            PdfValue xobjs = res.dictGet("XObject");
            std::string key = xobjPlaceholder.resourceName.toStdString();
            if (!key.empty() && key[0] == '/') key = key.substr(1);
            PdfValue ref = xobjs.dictGet(key);
            if (!ref.isRef()) {
                result.warnings.push_back(QString("XObject %1 not found in resources").arg(xobjPlaceholder.resourceName));
                continue;
            }
            const PdfObject* xo = doc.findObject(ref.refObj(), ref.refGen());
            if (!xo || !xo->isStream) {
                result.warnings.push_back(QString("XObject %1 not a stream").arg(xobjPlaceholder.resourceName));
                continue;
            }
            // Determine subtype
            PdfValue subtypeV = xo->value.dictGet("Subtype");
            QString subtype;
            if (subtypeV.isName()) subtype = subtypeV.nameValue();
            else if (subtypeV.isString()) subtype = QString::fromLatin1(subtypeV.stringValue());

            if (subtype == "Image") {
                auto obj = std::make_shared<ImageObject>();
                obj->setLayerId(layer->id());
                QByteArray raw((const char*)xo->stream.data(), (int)xo->stream.size());
                int w = xo->value.getInt("Width", 0);
                int h = xo->value.getInt("Height", 0);
                // Try JPEG via QImage::fromData first (works for DCTDecode)
                QImage decoded(raw);
                if (!decoded.isNull()) {
                    obj->setImage(decoded.scaled((int)xobjPlaceholder.displayWidth, (int)xobjPlaceholder.displayHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                } else if (w > 0 && h > 0 && w * h * 3 <= raw.size()) {
                    QImage tmp(w, h, QImage::Format_RGB888);
                    memcpy(tmp.bits(), raw.constData(), (size_t)tmp.sizeInBytes());
                    obj->setImage(tmp);
                } else {
                    // Fallback placeholder if cannot decode; keep the object so nothing is silently dropped
                    QImage placeholder((int)xobjPlaceholder.displayWidth, (int)xobjPlaceholder.displayHeight, QImage::Format_RGB32);
                    placeholder.fill(Qt::darkGray);
                    obj->setImage(placeholder);
                    result.warnings.push_back(QString("Image %1 could not be decoded (format unsupported); placeholder used").arg(xobjPlaceholder.resourceName));
                }
                obj->setTransform(xobjPlaceholder.transform);
                obj->setName(QString("Image %1").arg(++result.imagesImported));
                out.addObject(obj);
                layer->addObject(obj->id());
                ++result.objectsImported;
            } else if (subtype == "Form") {
                // Recurse into the form's content stream as a group
                auto groupObj = std::make_shared<GroupObject>();
                groupObj->setLayerId(layer->id());
                groupObj->setTransform(xobjPlaceholder.transform);
                groupObj->setName(QString("Form %1").arg(key.c_str()));

                // Form matrix (optional; default identity)
                QTransform formMatrix;
                PdfValue matrix = xo->value.dictGet("Matrix");
                if (matrix.isArray() && matrix.arraySize() >= 6) {
                    auto arr = matrix.array();
                    double a = arr[0].number(), b = arr[1].number(), c = arr[2].number(),
                           d = arr[3].number(), e = arr[4].number(), f = arr[5].number();
                    formMatrix = QTransform(a, b, c, d, e, f);
                }

                // Parse the form's content stream. PDF operators inside the form
                // are in the form's own user space, which is y-up and independent
                // from the enclosing page. Pass the form's own BBox as the y-flip
                // height so nested coordinates are QGraphicsView-compatible.
                double formHeight = 100.0;
                PdfValue bbox = xo->value.dictGet("BBox");
                if (bbox.isArray() && bbox.arraySize() >= 4) {
                    formHeight = bbox.arrayAt(3).number() - bbox.arrayAt(1).number();
                    if (formHeight <= 0.0) formHeight = 100.0;
                }
                QByteArray formContents((const char*)xo->stream.data(), (int)xo->stream.size());
                // We use identity initialCtm: the form's own CTM starts at identity
                // in PDF convention; the y-flip is applied with formHeight.
                PDF::ParseResult formParsed = PDF::PdfContentParser::parse(
                    formContents, doc, xo->value, QTransform(), formHeight);

                // Parse again isn't needed; we need to create objects. For simplicity, use a child layer.
                auto childLayer = std::make_shared<Layer>(QString("Form %1").arg(key.c_str()));
                out.addLayer(childLayer);
                // Add children to the child layer and to the group
                for (const auto& p : formParsed.paths) {
                    auto cobj = std::make_shared<PathObject>();
                    cobj->setLayerId(childLayer->id());
                    for (const auto& e : p.elements) {
                        switch (e.t) {
                            case PDF::ParsedPath::Element::T::Move:  cobj->addMove(e.p1); break;
                            case PDF::ParsedPath::Element::T::Line:  cobj->addLine(e.p1); break;
                            case PDF::ParsedPath::Element::T::Cubic: cobj->addCurve(e.p1, e.p2, e.p3); break;
                            case PDF::ParsedPath::Element::T::Close: cobj->closePath(); break;
                        }
                    }
                    cobj->fillStroke().strokeWidth = p.lineWidth;
                    cobj->fillStroke().capStyle = p.capStyle;
                    cobj->fillStroke().joinStyle = p.joinStyle;
                    cobj->fillStroke().miterLimit = (float)p.miterLimit;
                    cobj->fillStroke().dashPattern = p.dashPattern;
                    cobj->fillStroke().dashOffset = p.dashPhase;
                    if (p.hasFill) cobj->fillStroke().fill = p.fillColor;
                    if (p.hasStroke) cobj->fillStroke().stroke = p.strokeColor;
                    cobj->setName(QString("FormPath %1").arg(result.objectsImported + 1));
                    // Compose parent placeholder transform with form's own matrix and child local transform
                    cobj->setTransform(xobjPlaceholder.transform * formMatrix * cobj->transform());
                    out.addObject(cobj);
                    childLayer->addObject(cobj->id());
                    groupObj->addChild(cobj->id());
                    ++result.objectsImported;
                }
                for (const auto& t : formParsed.texts) {
                    auto cobj = std::make_shared<TextObject>();
                    cobj->setLayerId(childLayer->id());
                    cobj->setText(t.text);
                    QString fam = t.fontName;
                    bool bold = fam.contains("Bold", Qt::CaseInsensitive);
                    bool italic = fam.contains("Italic", Qt::CaseInsensitive) || fam.contains("Oblique", Qt::CaseInsensitive);
                    fam.replace("-Bold", "").replace("-Italic", "").replace("-Oblique", "");
                    if (fam.isEmpty()) fam = "Helvetica";
                    cobj->setFontFamily(fam);
                    cobj->setBold(bold);
                    cobj->setItalic(italic);
                    cobj->setFontSize(t.fontSize);
                    cobj->setTransform(xobjPlaceholder.transform * formMatrix * t.transform);
                    cobj->fillStroke().fill = t.fillColor;
                    cobj->setName(QString("FormText %1").arg(result.objectsImported + 1));
                    out.addObject(cobj);
                    childLayer->addObject(cobj->id());
                    groupObj->addChild(cobj->id());
                    ++result.objectsImported;
                }
                for (const auto& ftxt : formParsed.unsupported) {
                    result.warnings.push_back(QString("Form %1: %2").arg(key.c_str()).arg(ftxt));
                }
                out.addObject(groupObj);
                layer->addObject(groupObj->id());
                ++result.objectsImported;
            } else {
                result.warnings.push_back(QString("XObject %1: unsupported subtype '%2'").arg(xobjPlaceholder.resourceName, subtype));
            }
        }

        // Warn on annotations (not parsed, but we must not fail silently)
        {
            PdfValue pageVal = doc.pageDictionary(pageIndex);
            if (pageVal.dictHas("Annots")) {
                PdfValue annots = pageVal.dictGet("Annots");
                if (annots.isArray()) {
                    result.warnings.push_back(QString("Page %1 contains %2 annotation(s) — not yet supported").arg(pageIndex + 1).arg(annots.arraySize()));
                }
            }
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
