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

namespace {

std::shared_ptr<PathObject> pdfPathToObject(const PDF::ParsedPath& p) {
    auto obj = std::make_shared<PathObject>();
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
    obj->fillStroke().fillAlpha = p.fillAlpha;
    obj->fillStroke().strokeAlpha = p.strokeAlpha;
    if (p.hasFill) obj->fillStroke().fill = p.fillColor;
    if (p.hasStroke) obj->fillStroke().stroke = p.strokeColor;
    return obj;
}

std::shared_ptr<TextObject> pdfTextToObject(const PDF::ParsedText& t) {
    auto obj = std::make_shared<TextObject>();
    obj->setText(t.text);
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
    obj->fillStroke().fillAlpha = t.fillAlpha;
    return obj;
}

std::shared_ptr<ClippingMaskObject> pdfClipToObject(const PDF::ClippedArea& clip) {
    auto obj = std::make_shared<ClippingMaskObject>();
    obj->setEvenOdd(clip.evenOdd);
    for (const auto& e : clip.elements) {
        switch (e.t) {
            case PDF::ParsedPath::Element::T::Move:  obj->addMove(e.p1); break;
            case PDF::ParsedPath::Element::T::Line:  obj->addLine(e.p1); break;
            case PDF::ParsedPath::Element::T::Cubic: obj->addCurve(e.p1, e.p2, e.p3); break;
            case PDF::ParsedPath::Element::T::Close: obj->closePath(); break;
        }
    }
    return obj;
}

// Recursively imports an already-parsed result into the document, preserving
// q/Q scopes, clipping masks, and order of drawing.
// parentTransform: composed into each object's transform (identity at page level).
void importParsedInto(const PDF::PdfDocument& doc,
                      Document& out,
                      std::shared_ptr<Layer> targetLayer,
                      PDF::ParseResult& parsed,
                      const PdfValue& resources,
                      const QTransform& parentTransform,
                      const QString& namePrefix,
                      PdfImportResult& result) {
    // q/Q stack of scopes; each scope holds zero or more clip mask ids
    struct ClipScope { std::vector<ObjectId> clipIds; };
    std::vector<ClipScope> scopes;
    scopes.push_back(ClipScope{}); // page root scope

    auto activeClipMasks = [&]() -> std::vector<ObjectId> {
        std::vector<ObjectId> all;
        for (const auto& s : scopes) all.insert(all.end(), s.clipIds.begin(), s.clipIds.end());
        return all;
    };

    auto assignTransform = [&](const QTransform& local) -> QTransform {
        return parentTransform * local;
    };

    for (const auto& ev : parsed.sequence) {
        switch (ev.t) {
            case PDF::Event::T::Push:
                scopes.push_back(ClipScope{});
                break;

            case PDF::Event::T::Pop:
                if (scopes.size() > 1) scopes.pop_back();
                break;

            case PDF::Event::T::Clip: {
                auto mask = pdfClipToObject(parsed.clips[ev.index]);
                mask->setLayerId(targetLayer->id());
                mask->setTransform(parentTransform);
                mask->setName(QString("%1Clip %2").arg(namePrefix).arg(mask->id()));
                out.addObject(mask);
                targetLayer->addObject(mask->id());
                scopes.back().clipIds.push_back(mask->id());
                break;
            }

            case PDF::Event::T::Path: {
                const auto& p = parsed.paths[ev.index];
                auto obj = pdfPathToObject(p);
                obj->setLayerId(targetLayer->id());
                obj->setTransform(assignTransform(obj->transform()));
                obj->setClipMaskIds(activeClipMasks());
                obj->setName(QString("%1Path %2").arg(namePrefix).arg(++result.pathsImported));
                out.addObject(obj);
                targetLayer->addObject(obj->id());
                ++result.objectsImported;
                break;
            }

            case PDF::Event::T::Text: {
                const auto& t = parsed.texts[ev.index];
                auto obj = pdfTextToObject(t);
                obj->setLayerId(targetLayer->id());
                obj->setTransform(assignTransform(obj->transform()));
                obj->setClipMaskIds(activeClipMasks());
                obj->setName(QString("%1Text %2").arg(namePrefix).arg(++result.textsImported));
                out.addObject(obj);
                targetLayer->addObject(obj->id());
                ++result.objectsImported;
                break;
            }

            case PDF::Event::T::Image: {
                const auto& xobjPlaceholder = parsed.images[ev.index];
                // Resolve XObject dict from the given resources
                PdfValue xobjs = resources.dictGet("XObject");
                std::string key = xobjPlaceholder.resourceName.toStdString();
                if (!key.empty() && key[0] == '/') key = key.substr(1);
                PdfValue ref = xobjs.dictGet(key);
                if (!ref.isRef()) {
                    result.warnings.push_back(QString("XObject %1 not found in resources").arg(xobjPlaceholder.resourceName));
                    break;
                }
                const PdfObject* xo = doc.findObject(ref.refObj(), ref.refGen());
                if (!xo || !xo->isStream) {
                    result.warnings.push_back(QString("XObject %1 not a stream").arg(xobjPlaceholder.resourceName));
                    break;
                }
                PdfValue subtypeV = xo->value.dictGet("Subtype");
                QString subtype;
                if (subtypeV.isName()) subtype = subtypeV.nameValue();
                else if (subtypeV.isString()) subtype = QString::fromLatin1(subtypeV.stringValue());

                if (subtype == "Image") {
                    auto obj = std::make_shared<ImageObject>();
                    obj->setLayerId(targetLayer->id());
                    QByteArray raw((const char*)xo->stream.data(), (int)xo->stream.size());
                    int w = xo->value.getInt("Width", 0);
                    int h = xo->value.getInt("Height", 0);
                    // Try JPEG/encoded data via QImage::fromData first (DCTDecode etc.)
                    QImage decoded(raw);
                    if (!decoded.isNull()) {
                        obj->setImage(decoded.scaled((int)xobjPlaceholder.displayWidth, (int)xobjPlaceholder.displayHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                    } else if (w > 0 && h > 0 && (int64_t)w * h * 3 <= raw.size()) {
                        QImage tmp(w, h, QImage::Format_RGB888);
                        memcpy(tmp.bits(), raw.constData(), (size_t)tmp.sizeInBytes());
                        obj->setImage(tmp);
                    } else {
                        // Record as unsupported; do not fabricate a visible object silently
                        result.warnings.push_back(QString("Image %1 could not be decoded (w=%2 h=%3); object omitted from model, original content preserved in source PDF").arg(xobjPlaceholder.resourceName).arg(w).arg(h));
                        break;
                    }
                    obj->setTransform(assignTransform(xobjPlaceholder.transform));
                    obj->setClipMaskIds(activeClipMasks());
                    obj->fillStroke().fillAlpha = xobjPlaceholder.opacity;
                    obj->setName(QString("%1Image %2").arg(namePrefix).arg(++result.imagesImported));
                    out.addObject(obj);
                    targetLayer->addObject(obj->id());
                    ++result.objectsImported;
                } else if (subtype == "Form") {
                    // Recurse into the form's content stream as a group
                    auto groupObj = std::make_shared<GroupObject>();
                    groupObj->setLayerId(targetLayer->id());
                    groupObj->setTransform(assignTransform(xobjPlaceholder.transform));
                    groupObj->setName(QString("%1Form %2").arg(namePrefix, QString::fromStdString(key)));
                    groupObj->setClipMaskIds(activeClipMasks());

                    // Form matrix (optional; PDF /Matrix)
                    QTransform formMatrix;
                    PdfValue matrix = xo->value.dictGet("Matrix");
                    if (matrix.isArray() && matrix.arraySize() >= 6) {
                        auto arr = matrix.array();
                        double a = arr[0].number(), b = arr[1].number(), c = arr[2].number(),
                               d = arr[3].number(), e = arr[4].number(), f = arr[5].number();
                        formMatrix = QTransform(a, b, c, d, e, f);
                    }

                    // The form content uses its own user space; the y-flip height
                    // is derived from the form BBox.
                    double formHeight = 100.0;
                    PdfValue bbox = xo->value.dictGet("BBox");
                    if (bbox.isArray() && bbox.arraySize() >= 4) {
                        formHeight = bbox.arrayAt(3).number() - bbox.arrayAt(1).number();
                        if (formHeight <= 0.0) formHeight = 100.0;
                    }

                    // Compose parent transform with the form's own matrix; children keep identity.
                    QTransform formCompose = assignTransform(xobjPlaceholder.transform) * formMatrix;

                    auto childLayer = std::make_shared<Layer>(QString("Form %1").arg(QString::fromStdString(key)));
                    out.addLayer(childLayer);

                    QByteArray formContents((const char*)xo->stream.data(), (int)xo->stream.size());
                    PDF::ParseResult formParsed = PDF::PdfContentParser::parse(
                        formContents, doc, xo->value, QTransform(), formHeight);

                    size_t objectsBeforeGroup = result.objectsImported;
                    importParsedInto(doc, out, childLayer, formParsed,
                                     xo->value, formCompose,
                                     namePrefix + QString::fromStdString(key) + "/",
                                     result);

                    // Attach the produced children to the group
                    for (size_t i = objectsBeforeGroup; i < result.objectsImported; ++i) {
                        // Groups are added to layers by importParsedInto; gather ObjectIds
                        // from the child layer's object list (last objects added).
                        // Child layer holds ids in order; attach the tail.
                    }
                    Q_UNUSED(objectsBeforeGroup);
                    // The child layer owns the new children; group simply records hierarchy by id references
                    for (ObjectId oid : childLayer->objects()) groupObj->addChild(oid);

                    for (const auto& ftxt : formParsed.unsupported) {
                        result.warnings.push_back(QString("Form %1: %2").arg(QString::fromStdString(key), ftxt));
                    }

                    out.addObject(groupObj);
                    targetLayer->addObject(groupObj->id());
                    ++result.objectsImported;
                } else {
                    result.warnings.push_back(QString("XObject %1: unsupported subtype '%2'").arg(xobjPlaceholder.resourceName, subtype));
                }
                break;
            }
        }
    }
}

} // anonymous namespace

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

        auto page = std::make_shared<Page>();
        page->boxes().mediaBox = QRectF(0, 0, pageWidth, pageHeight);
        page->boxes().cropBox  = QRectF(0, 0, pageWidth, pageHeight);
        page->boxes().bleedBox = QRectF(0, 0, pageWidth, pageHeight);
        page->boxes().trimBox  = QRectF(0, 0, pageWidth, pageHeight);
        page->boxes().artBox   = QRectF(0, 0, pageWidth, pageHeight);

        // One layer per page to keep the hierarchy inspectable
        auto layer = std::make_shared<Layer>(QString("Page %1").arg(pageIndex + 1));
        out.addLayer(layer);
        page->addLayer(layer->id());
        out.addPage(page);

        // Parse content stream in drawing order
        QByteArray contents = doc.pageContents(pageIndex);
        PdfValue pageResources = doc.pageResources(pageIndex);
        PDF::ParseResult parsed = PDF::PdfContentParser::parse(contents, doc, pageResources, QTransform(), pageHeight);

        for (const auto& unsup : parsed.unsupported) {
            result.warnings.push_back(unsup);
        }

        importParsedInto(doc, out, layer, parsed, pageResources, QTransform(),
                         QString(), result);

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

        ++result.pagesImported;
    }

    result.ok = true;
    return result;
}

} // namespace ArtPlus
