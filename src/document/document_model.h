#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <QRectF>
#include <QColor>
#include <QTransform>
#include <QUuid>

namespace ArtPlus {

class DocumentObject;
class Layer;
class Page;
class ColorSpace;
class Separation;

using ObjectId = QUuid;
using LayerId = QUuid;
using PageId = QUuid;

enum class ObjectType {
    Path,
    Text,
    Image,
    Group,
    ClippingMask,
    OpacityMask
};

enum class PageBoxType {
    MediaBox,
    CropBox,
    BleedBox,
    TrimBox,
    ArtBox
};

struct PageBoxes {
    QRectF mediaBox;
    QRectF cropBox;
    QRectF bleedBox;
    QRectF trimBox;
    QRectF artBox;

    PageBoxes() : mediaBox(0, 0, 595, 842), cropBox(0, 0, 595, 842),
                  bleedBox(0, 0, 595, 842), trimBox(0, 0, 595, 842),
                  artBox(0, 0, 595, 842) {}
};

struct Color {
    enum class Type { CMYK, Spot, RGB, Lab, Gray, Pattern, None } type = Type::None;
    QVector<float> components;
    QString spotName;
    float tint = 1.0f;

    Color() = default;
    static Color cmyk(float c, float m, float y, float k);
    static Color spot(const QString& name, float tint = 1.0f);
    static Color rgb(float r, float g, float b);
    bool isValid() const { return type != Type::None; }
};

struct FillStroke {
    Color fill;
    Color stroke;
    float strokeWidth = 1.0f;
    Qt::PenCapStyle capStyle = Qt::FlatCap;
    Qt::PenJoinStyle joinStyle = Qt::MiterJoin;
    float miterLimit = 10.0f;
    std::vector<float> dashPattern;
    float dashOffset = 0.0f;
};

struct GradientStop {
    float offset;
    Color color;
};

struct Gradient {
    enum class Type { Linear, Radial, Freeform } type = Type::Linear;
    QVector<GradientStop> stops;
    QPointF startPoint;
    QPointF endPoint;
    float radius = 0.0f;
    QTransform matrix;
    bool isValid() const { return !stops.isEmpty(); }
};

class DocumentObject : public std::enable_shared_from_this<DocumentObject> {
public:
    DocumentObject(ObjectType type) : m_type(type), m_id(QUuid::createUuid()) {}
    virtual ~DocumentObject() = default;

    ObjectId id() const { return m_id; }
    ObjectType type() const { return m_type; }
    const QString& name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    bool visible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    bool locked() const { return m_locked; }
    void setLocked(bool l) { m_locked = l; }

    const QTransform& transform() const { return m_transform; }
    void setTransform(const QTransform& t) { m_transform = t; }
    void applyTransform(const QTransform& t) { m_transform = t * m_transform; }

    const FillStroke& fillStroke() const { return m_fillStroke; }
    FillStroke& fillStroke() { return m_fillStroke; }

    virtual QRectF boundingBox() const = 0;
    virtual std::shared_ptr<DocumentObject> clone() const = 0;

    LayerId layerId() const { return m_layerId; }
    void setLayerId(LayerId id) { m_layerId = id; }

    ObjectId parentId() const { return m_parentId; }
    void setParentId(ObjectId id) { m_parentId = id; }

    const std::vector<ObjectId>& children() const { return m_children; }
    void addChild(ObjectId id) { m_children.push_back(id); }
    void removeChild(ObjectId id);

private:
    ObjectId m_id;
    ObjectType m_type;
    QString m_name;
    bool m_visible = true;
    bool m_locked = false;
    QTransform m_transform;
    FillStroke m_fillStroke;
    LayerId m_layerId;
    ObjectId m_parentId;
    std::vector<ObjectId> m_children;
};

class PathObject : public DocumentObject {
public:
    PathObject() : DocumentObject(ObjectType::Path) {}

    struct PathElement {
        enum class Type { Move, Line, Curve, Close } type;
        QPointF p1, p2, p3;
    };

    const QVector<PathElement>& elements() const { return m_elements; }
    QVector<PathElement>& elements() { return m_elements; }
    void addMove(const QPointF& p);
    void addLine(const QPointF& p);
    void addCurve(const QPointF& c1, const QPointF& c2, const QPointF& end);
    void closePath();

    QRectF boundingBox() const override;
    std::shared_ptr<DocumentObject> clone() const override;

    bool isClosed() const { return m_closed; }
    void setClosed(bool c) { m_closed = c; }

    std::optional<Gradient> fillGradient;
    std::optional<Gradient> strokeGradient;

private:
    QVector<PathElement> m_elements;
    bool m_closed = false;
};

class TextObject : public DocumentObject {
public:
    TextObject() : DocumentObject(ObjectType::Text) {}

    QString text() const { return m_text; }
    void setText(const QString& t) { m_text = t; }

    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& f) { m_fontFamily = f; }
    float fontSize() const { return m_fontSize; }
    void setFontSize(float s) { m_fontSize = s; }
    bool bold() const { return m_bold; }
    void setBold(bool b) { m_bold = b; }
    bool italic() const { return m_italic; }
    void setItalic(bool i) { m_italic = i; }

    QRectF boundingBox() const override;
    std::shared_ptr<DocumentObject> clone() const override;

private:
    QString m_text;
    QString m_fontFamily = "Arial";
    float m_fontSize = 12.0f;
    bool m_bold = false;
    bool m_italic = false;
};

class ImageObject : public DocumentObject {
public:
    ImageObject() : DocumentObject(ObjectType::Image) {}

    const QImage& image() const { return m_image; }
    void setImage(const QImage& img) { m_image = img; }

    QRectF boundingBox() const override;
    std::shared_ptr<DocumentObject> clone() const override;

private:
    QImage m_image;
};

class GroupObject : public DocumentObject {
public:
    GroupObject() : DocumentObject(ObjectType::Group) {}

    QRectF boundingBox() const override;
    std::shared_ptr<DocumentObject> clone() const override;
};

class Layer {
public:
    Layer(const QString& name = "Layer") : m_name(name), m_id(QUuid::createUuid()) {}

    LayerId id() const { return m_id; }
    const QString& name() const { return m_name; }
    void setName(const QString& n) { m_name = n; }

    bool visible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    bool locked() const { return m_locked; }
    void setLocked(bool l) { m_locked = l; }
    bool printable() const { return m_printable; }
    void setPrintable(bool p) { m_printable = p; }

    const QColor& color() const { return m_color; }
    void setColor(const QColor& c) { m_color = c; }

    float opacity() const { return m_opacity; }
    void setOpacity(float o) { m_opacity = std::clamp(o, 0.0f, 1.0f); }

    const std::vector<ObjectId>& objects() const { return m_objects; }
    void addObject(ObjectId id) { m_objects.push_back(id); }
    void removeObject(ObjectId id);

private:
    LayerId m_id;
    QString m_name;
    bool m_visible = true;
    bool m_locked = false;
    bool m_printable = true;
    QColor m_color = Qt::blue;
    float m_opacity = 1.0f;
    std::vector<ObjectId> m_objects;
};

class Page {
public:
    Page() : m_id(QUuid::createUuid()) {}

    PageId id() const { return m_id; }
    const PageBoxes& boxes() const { return m_boxes; }
    PageBoxes& boxes() { return m_boxes; }

    const std::vector<LayerId>& layers() const { return m_layers; }
    void addLayer(LayerId id) { m_layers.push_back(id); }

    QRectF pageRect() const { return m_boxes.cropBox; }

private:
    PageId m_id;
    PageBoxes m_boxes;
    std::vector<LayerId> m_layers;
};

class Document {
public:
    Document() = default;

    void addPage(std::shared_ptr<Page> page);
    void removePage(PageId id);
    std::shared_ptr<Page> page(PageId id);
    const std::vector<PageId>& pageOrder() const { return m_pageOrder; }
    std::shared_ptr<Page> currentPage() const;

    void addLayer(std::shared_ptr<Layer> layer);
    void removeLayer(LayerId id);
    std::shared_ptr<Layer> layer(LayerId id);
    const std::vector<LayerId>& layerOrder() const { return m_layerOrder; }

    void addObject(std::shared_ptr<DocumentObject> obj);
    void removeObject(ObjectId id);
    std::shared_ptr<DocumentObject> object(ObjectId id);
    const std::unordered_map<ObjectId, std::shared_ptr<DocumentObject>>& objects() const { return m_objects; }

    std::vector<ObjectId> selectedObjects() const { return m_selection; }
    void selectObject(ObjectId id, bool add = false);
    void deselectObject(ObjectId id);
    void clearSelection();

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path) { m_filePath = path; }

    bool modified() const { return m_modified; }
    void setModified(bool m) { m_modified = m; }

private:
    std::vector<PageId> m_pageOrder;
    std::unordered_map<PageId, std::shared_ptr<Page>> m_pages;

    std::vector<LayerId> m_layerOrder;
    std::unordered_map<LayerId, std::shared_ptr<Layer>> m_layers;

    std::unordered_map<ObjectId, std::shared_ptr<DocumentObject>> m_objects;
    std::vector<ObjectId> m_selection;

    QString m_filePath;
    bool m_modified = false;
};

} // namespace ArtPlus