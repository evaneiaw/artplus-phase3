#include "document_serializer.h"
#include <QFile>
#include <QJsonArray>
#include <QBuffer>
#include <algorithm>
#include <memory>

namespace ArtPlus {

// =============================================================== helpers
static QJsonObject colorToJson(const Color& c) {
    QJsonObject o;
    o["type"] = (int)c.type;
    QJsonArray comps;
    for (float v : c.components) comps.append((double)v);
    o["components"] = comps;
    o["spotName"] = c.spotName;
    o["tint"] = (double)c.tint;
    return o;
}

static Color colorFromJson(const QJsonObject& o) {
    Color c;
    c.type = (Color::Type)o.value("type").toInt(0);
    for (const auto& v : o.value("components").toArray()) c.components.append((float)v.toDouble());
    c.spotName = o.value("spotName").toString();
    c.tint = (float)o.value("tint").toDouble(1.0);
    return c;
}

static QJsonObject fillStrokeToJson(const FillStroke& fs) {
    QJsonObject o;
    o["fill"] = colorToJson(fs.fill);
    o["stroke"] = colorToJson(fs.stroke);
    o["strokeWidth"] = (double)fs.strokeWidth;
    o["capStyle"] = (int)fs.capStyle;
    o["joinStyle"] = (int)fs.joinStyle;
    o["miterLimit"] = (double)fs.miterLimit;
    QJsonArray dash;
    for (float v : fs.dashPattern) dash.append((double)v);
    o["dashPattern"] = dash;
    o["dashOffset"] = (double)fs.dashOffset;
    o["fillAlpha"] = (double)fs.fillAlpha;
    o["strokeAlpha"] = (double)fs.strokeAlpha;
    return o;
}

static FillStroke fillStrokeFromJson(const QJsonObject& o) {
    FillStroke fs;
    fs.fill = colorFromJson(o.value("fill").toObject());
    fs.stroke = colorFromJson(o.value("stroke").toObject());
    fs.strokeWidth = (float)o.value("strokeWidth").toDouble(1.0);
    fs.capStyle = (Qt::PenCapStyle)o.value("capStyle").toInt(0);
    fs.joinStyle = (Qt::PenJoinStyle)o.value("joinStyle").toInt(0);
    fs.miterLimit = (float)o.value("miterLimit").toDouble(10.0);
    for (const auto& v : o.value("dashPattern").toArray()) fs.dashPattern.push_back((float)v.toDouble());
    fs.dashOffset = (float)o.value("dashOffset").toDouble(0.0);
    fs.fillAlpha = (float)o.value("fillAlpha").toDouble(1.0);
    fs.strokeAlpha = (float)o.value("strokeAlpha").toDouble(1.0);
    return fs;
}

static QJsonObject transformToJson(const QTransform& t) {
    // QTransform::m11..m33 mapping: store as flat array (PDF-style affine subset)
    QJsonObject o;
    o["m11"] = t.m11(); o["m12"] = t.m12(); o["m13"] = t.m13();
    o["m21"] = t.m21(); o["m22"] = t.m22(); o["m23"] = t.m23();
    o["m31"] = t.m31(); o["m32"] = t.m32(); o["m33"] = t.m33();
    return o;
}

static QTransform transformFromJson(const QJsonObject& o) {
    return QTransform(o.value("m11").toDouble(1), o.value("m12").toDouble(0), o.value("m13").toDouble(0),
                      o.value("m21").toDouble(0), o.value("m22").toDouble(1), o.value("m23").toDouble(0),
                      o.value("m31").toDouble(0), o.value("m32").toDouble(0), o.value("m33").toDouble(1));
}

static QJsonObject rectToJson(const QRectF& r) {
    QJsonObject o;
    o["x"] = r.x(); o["y"] = r.y(); o["w"] = r.width(); o["h"] = r.height();
    return o;
}

static QRectF rectFromJson(const QJsonObject& o) {
    return QRectF(o.value("x").toDouble(), o.value("y").toDouble(),
                  o.value("w").toDouble(), o.value("h").toDouble());
}

static QJsonObject boxesToJson(const PageBoxes& b) {
    QJsonObject o;
    o["mediaBox"] = rectToJson(b.mediaBox);
    o["cropBox"]  = rectToJson(b.cropBox);
    o["bleedBox"] = rectToJson(b.bleedBox);
    o["trimBox"]  = rectToJson(b.trimBox);
    o["artBox"]   = rectToJson(b.artBox);
    return o;
}

static PageBoxes boxesFromJson(const QJsonObject& o) {
    PageBoxes b;
    b.mediaBox = rectFromJson(o.value("mediaBox").toObject());
    b.cropBox  = rectFromJson(o.value("cropBox").toObject());
    b.bleedBox = rectFromJson(o.value("bleedBox").toObject());
    b.trimBox  = rectFromJson(o.value("trimBox").toObject());
    b.artBox   = rectFromJson(o.value("artBox").toObject());
    return b;
}

namespace {
struct TypeTag {
    static QString name(ObjectType t) {
        switch (t) {
            case ObjectType::Path: return "Path";
            case ObjectType::Text: return "Text";
            case ObjectType::Image: return "Image";
            case ObjectType::Group: return "Group";
            case ObjectType::ClippingMask: return "ClippingMask";
            case ObjectType::OpacityMask: return "OpacityMask";
        }
        return "Unknown";
    }
    static ObjectType from(const QString& s) {
        if (s == "Path") return ObjectType::Path;
        if (s == "Text") return ObjectType::Text;
        if (s == "Image") return ObjectType::Image;
        if (s == "Group") return ObjectType::Group;
        if (s == "ClippingMask") return ObjectType::ClippingMask;
        if (s == "OpacityMask") return ObjectType::OpacityMask;
        return ObjectType::Path;
    }
};
} // anonymous

static QJsonObject objectToJson(const DocumentObject& obj) {
    QJsonObject o;
    o["type"] = TypeTag::name(obj.type());
    o["id"] = (int)obj.id();
    o["name"] = obj.name();
    o["visible"] = obj.visible();
    o["locked"] = obj.locked();
    o["transform"] = transformToJson(obj.transform());
    o["fillStroke"] = fillStrokeToJson(obj.fillStroke());
    o["layerId"] = (int)obj.layerId();
    o["parentId"] = (int)obj.parentId();
    QJsonArray clipMasks;
    for (ObjectId c : obj.clipMaskIds()) clipMasks.append((int)c);
    o["clipMasks"] = clipMasks;
    QJsonArray children;
    for (ObjectId c : obj.children()) children.append((int)c);
    o["children"] = children;

    // Type-specific
    if (obj.type() == ObjectType::Path) {
        auto& p = static_cast<const PathObject&>(obj);
        QJsonArray elems;
        for (const auto& e : p.elements()) {
            QJsonObject eo;
            eo["t"] = (int)e.type;
            eo["p1x"] = e.p1.x(); eo["p1y"] = e.p1.y();
            eo["p2x"] = e.p2.x(); eo["p2y"] = e.p2.y();
            eo["p3x"] = e.p3.x(); eo["p3y"] = e.p3.y();
            elems.append(eo);
        }
        o["elements"] = elems;
        o["closed"] = p.isClosed();
    } else if (obj.type() == ObjectType::ClippingMask) {
        auto& mask = static_cast<const ClippingMaskObject&>(obj);
        QJsonArray elems;
        for (const auto& e : mask.elements()) {
            QJsonObject eo;
            eo["t"] = (int)e.t;
            eo["p1x"] = e.p1.x(); eo["p1y"] = e.p1.y();
            eo["p2x"] = e.p2.x(); eo["p2y"] = e.p2.y();
            eo["p3x"] = e.p3.x(); eo["p3y"] = e.p3.y();
            elems.append(eo);
        }
        o["elements"] = elems;
        o["evenOdd"] = mask.evenOdd();
    } else if (obj.type() == ObjectType::Text) {
        auto& t = static_cast<const TextObject&>(obj);
        o["text"] = t.text();
        o["fontFamily"] = t.fontFamily();
        o["fontSize"] = t.fontSize();
        o["bold"] = t.bold();
        o["italic"] = t.italic();
    } else if (obj.type() == ObjectType::Image) {
        auto& img = static_cast<const ImageObject&>(obj);
        o["imageWidth"] = img.image().width();
        o["imageHeight"] = img.image().height();
        // encode as PNG base64 for round-trip integrity
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        img.image().save(&buf, "PNG");
        o["imagePng"] = QString::fromLatin1(ba.toBase64());
    }
    return o;
}

static std::shared_ptr<DocumentObject> objectFromJson(const QJsonObject& o) {
    ObjectType type = TypeTag::from(o.value("type").toString());
    std::shared_ptr<DocumentObject> obj;
    if (type == ObjectType::Path) {
        auto p = std::make_shared<PathObject>();
        for (const auto& ev : o.value("elements").toArray()) {
            QJsonObject e = ev.toObject();
            PathObject::PathElement el;
            el.type = (PathObject::PathElement::Type)e.value("t").toInt();
            el.p1 = QPointF(e.value("p1x").toDouble(), e.value("p1y").toDouble());
            el.p2 = QPointF(e.value("p2x").toDouble(), e.value("p2y").toDouble());
            el.p3 = QPointF(e.value("p3x").toDouble(), e.value("p3y").toDouble());
            p->elements().append(el);
        }
        p->setClosed(o.value("closed").toBool());
        obj = p;
    } else if (type == ObjectType::ClippingMask) {
        auto mask = std::make_shared<ClippingMaskObject>();
        for (const auto& ev : o.value("elements").toArray()) {
            QJsonObject e = ev.toObject();
            ClippingMaskObject::Element el;
            el.t = (ClippingMaskObject::Element::T)e.value("t").toInt();
            el.p1 = QPointF(e.value("p1x").toDouble(), e.value("p1y").toDouble());
            el.p2 = QPointF(e.value("p2x").toDouble(), e.value("p2y").toDouble());
            el.p3 = QPointF(e.value("p3x").toDouble(), e.value("p3y").toDouble());
            mask->elements().push_back(el);
        }
        mask->setEvenOdd(o.value("evenOdd").toBool());
        obj = mask;
    } else if (type == ObjectType::Text) {
        auto t = std::make_shared<TextObject>();
        t->setText(o.value("text").toString());
        t->setFontFamily(o.value("fontFamily").toString());
        t->setFontSize((float)o.value("fontSize").toDouble(12.0));
        t->setBold(o.value("bold").toBool());
        t->setItalic(o.value("italic").toBool());
        obj = t;
    } else if (type == ObjectType::Image) {
        auto img = std::make_shared<ImageObject>();
        QByteArray ba = QByteArray::fromBase64(o.value("imagePng").toString().toLatin1());
        QImage qimg;
        qimg.loadFromData(ba, "PNG");
        img->setImage(qimg);
        obj = img;
    } else if (type == ObjectType::Group) {
        obj = std::make_shared<GroupObject>();
    } else {
        obj = std::make_shared<PathObject>();
    }

    // common props
    obj->setName(o.value("name").toString());
    obj->setVisible(o.value("visible").toBool(true));
    obj->setLocked(o.value("locked").toBool(false));
    obj->setTransform(transformFromJson(o.value("transform").toObject()));
    obj->fillStroke() = fillStrokeFromJson(o.value("fillStroke").toObject());
    obj->setLayerId((LayerId)o.value("layerId").toInt());
    obj->setParentId((ObjectId)o.value("parentId").toInt());
    QJsonArray clipMasks = o.value("clipMasks").toArray();
    for (const auto& cv : clipMasks) obj->addClipMaskId((ObjectId)cv.toInt());
    QJsonArray children = o.value("children").toArray();
    for (const auto& cv : children) obj->addChild((ObjectId)cv.toInt());
    // restore explicit id (monotonic counter already moved past it)
    obj->setId((ObjectId)o.value("id").toInt());
    return obj;
}

QJsonObject DocumentSerializer::toJson(const Document& doc) {
    QJsonObject root;
    root["format"] = "artplus";
    root["version"] = 1;
    root["filePath"] = doc.filePath();
    root["modified"] = doc.modified();

    // Layers
    QJsonArray layers;
    for (LayerId id : doc.layerOrder()) {
        auto layer = doc.layer(id);
        if (!layer) continue;
        QJsonObject lo;
        lo["id"] = (int)layer->id();
        lo["name"] = layer->name();
        lo["visible"] = layer->visible();
        lo["locked"] = layer->locked();
        lo["printable"] = layer->printable();
        lo["opacity"] = layer->opacity();
        QJsonArray objs;
        for (ObjectId oid : layer->objects()) objs.append((int)oid);
        lo["objects"] = objs;
        layers.append(lo);
    }
    root["layers"] = layers;

    // Pages
    QJsonArray pages;
    for (PageId id : doc.pageOrder()) {
        auto page = doc.page(id);
        if (!page) continue;
        QJsonObject po;
        po["id"] = (int)page->id();
        po["boxes"] = boxesToJson(page->boxes());
        QJsonArray pls;
        for (LayerId lid : page->layers()) pls.append((int)lid);
        po["layers"] = pls;
        pages.append(po);
    }
    root["pages"] = pages;

    // Objects
    QJsonArray objects;
    for (const auto& [id, obj] : doc.objects()) {
        objects.append(objectToJson(*obj));
    }
    root["objects"] = objects;

    // Selection
    QJsonArray sel;
    for (ObjectId id : doc.selectedObjects()) sel.append((int)id);
    root["selection"] = sel;

    return root;
}

bool DocumentSerializer::fromJson(const QJsonObject& json, Document& out) {
    if (json.value("format").toString() != "artplus") return false;

    // Strategy: load layers/pages/objects with fresh ids and remap.
    QJsonObject root = json;

    // First pass: create layers, remember old->new mapping
    std::unordered_map<int, LayerId> layerMap;
    std::unordered_map<int, PageId> pageMap;
    std::unordered_map<int, ObjectId> objectMap;

    QJsonArray layersArr = root.value("layers").toArray();
    for (const auto& v : layersArr) {
        QJsonObject lo = v.toObject();
        int oldId = lo.value("id").toInt();
        auto layer = std::make_shared<Layer>(lo.value("name").toString());
        layer->setVisible(lo.value("visible").toBool(true));
        layer->setLocked(lo.value("locked").toBool(false));
        layer->setPrintable(lo.value("printable").toBool(true));
        layer->setOpacity((float)lo.value("opacity").toDouble(1.0));
        // capture the new id assigned by constructor
        layerMap[oldId] = layer->id();
        out.addLayer(layer);
    }

    // Objects
    QJsonArray objectsArr = root.value("objects").toArray();
    // Load in id order for deterministic mapping
    std::vector<QJsonObject> objs;
    for (const auto& v : objectsArr) objs.push_back(v.toObject());
    for (const auto& o : objs) {
        auto obj = objectFromJson(o);
        objectMap[o.value("id").toInt()] = obj->id();
        // remap layer id; also register the object in the layer's member list
        int oldLayer = o.value("layerId").toInt();
        if (layerMap.count(oldLayer)) {
            obj->setLayerId(layerMap[oldLayer]);
            if (auto layer = out.layer(layerMap[oldLayer])) layer->addObject(obj->id());
        }
        int oldParent = o.value("parentId").toInt();
        (void)oldParent;
        out.addObject(obj);
    }
    // children + clip mask remap (ids change on load; rewrite references)
    for (const auto& o : objs) {
        int oldObjId = o.value("id").toInt();
        ObjectId newObjId = objectMap[oldObjId];
        auto obj = out.object(newObjId);
        if (!obj) continue;

        // Remap children: drop stored (old) ids, re-add mapped ids
        QJsonArray children = o.value("children").toArray();
        std::vector<ObjectId> oldIds;
        for (const auto& cv : children) oldIds.push_back((ObjectId)cv.toInt());
        for (ObjectId oldChild : oldIds) obj->removeChild(oldChild);
        for (ObjectId oldChild : oldIds) {
            auto it = objectMap.find((int)oldChild);
            if (it != objectMap.end()) obj->addChild(it->second);
        }

        // Remap clip mask references
        QJsonArray clipMasks = o.value("clipMasks").toArray();
        std::vector<ObjectId> newMasks;
        for (const auto& cv : clipMasks) {
            int oldMask = cv.toInt();
            auto it = objectMap.find(oldMask);
            if (it != objectMap.end()) newMasks.push_back(it->second);
        }
        obj->setClipMaskIds(newMasks);

        // Remap parent id
        int oldParent = o.value("parentId").toInt();
        if (oldParent >= 0) {
            auto it = objectMap.find(oldParent);
            if (it != objectMap.end()) obj->setParentId(it->second);
            else obj->setParentId(0);
        } else {
            obj->setParentId(0);
        }
    }

    // Pages
    QJsonArray pagesArr = root.value("pages").toArray();
    for (const auto& v : pagesArr) {
        QJsonObject po = v.toObject();
        auto page = std::make_shared<Page>();
        page->boxes() = boxesFromJson(po.value("boxes").toObject());
        QJsonArray pls = po.value("layers").toArray();
        for (const auto& lv : pls) {
            int oldLayerId = lv.toInt();
            if (layerMap.count(oldLayerId)) page->addLayer(layerMap[oldLayerId]);
        }
        // create pageId mapping
        pageMap[po.value("id").toInt()] = page->id();
        out.addPage(page);
    }

    out.setFilePath(root.value("filePath").toString());
    out.setModified(false);
    return true;
}

bool DocumentSerializer::save(const Document& doc, const QString& filePath) {
    QJsonObject json = toJson(doc);
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Indented);
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(data);
    return true;
}

bool DocumentSerializer::load(const QString& filePath, Document& out) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data = f.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return false;
    return fromJson(doc.object(), out);
}

DocumentSerializer::IntegrityReport DocumentSerializer::compare(const Document& a, const Document& b) {
    IntegrityReport report;
    auto check = [&](const QString& label, auto&& fn) {
        auto va = fn(a);
        auto vb = fn(b);
        if (va != vb) {
            report.ok = false;
            report.differences.append(QString("%1: %2 != %3").arg(label).arg(va).arg(vb));
        }
    };
    check("page count", [](const Document& d) { return d.pageOrder().size(); });
    check("layer count", [](const Document& d) { return d.layerOrder().size(); });
    check("object count", [](const Document& d) { return d.objects().size(); });
    check("selection count", [](const Document& d) { return d.selectedObjects().size(); });

    // Compare per-page boxes
    for (size_t i = 0; i < a.pageOrder().size() && i < b.pageOrder().size(); ++i) {
        auto pa = a.page(a.pageOrder()[i]);
        auto pb = b.page(b.pageOrder()[i]);
        if (!pa || !pb) { report.ok = false; report.differences.append(QString("page %1 missing").arg(i)); continue; }
        auto cmp = [&](const QString& label, const QRectF& ra, const QRectF& rb) {
            if (ra != rb) {
                report.ok = false;
                report.differences.append(QString("page %1 %2: (%3) vs (%4)").arg(i).arg(label).arg(ra.x()).arg(rb.x()));
            }
        };
        cmp("mediaBox", pa->boxes().mediaBox, pb->boxes().mediaBox);
        cmp("cropBox", pa->boxes().cropBox, pb->boxes().cropBox);
    }

    // Compare object types/names/transforms
    QList<ObjectId> ids = QList<ObjectId>();
    for (ObjectId id : a.objectIds()) ids.append(id);
    std::sort(ids.begin(), ids.end());
    for (ObjectId id : ids) {
        auto oa = a.object(id);
        auto ob = b.object(id);
        if (!ob) { report.ok = false; report.differences.append(QString("object %1 missing in B").arg(id)); continue; }
        if (oa->type() != ob->type()) { report.ok = false; report.differences.append(QString("object %1 type mismatch").arg(id)); }
        if (oa->name() != ob->name()) { report.ok = false; report.differences.append(QString("object %1 name mismatch").arg(id)); }
        if (oa->transform() != ob->transform()) { report.ok = false; report.differences.append(QString("object %1 transform mismatch").arg(id)); }
    }

    return report;
}

} // namespace ArtPlus
