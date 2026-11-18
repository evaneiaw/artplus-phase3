#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <QRectF>
#include <QColor>
#include <QTransform>
#include <QUuid>
#include <QImage>
#include <QVector>

namespace ArtPlus {

// Helper for std::hash with QUuid
struct QUuidHash {
    std::size_t operator()(const QUuid &id) const noexcept {
        // Simple hash based on the string representation
        std::size_t h = 0;
        const char* data = id.toString().toUtf8().constData();
        for (size_t i = 0; data[i]; ++i) {
            h = h * 31 + data[i];
        }
        return h;
    }
};

class DocumentObject;
class Layer;
class Page;
class ColorSpace;
class Separation;

using ObjectId = uint32_t;
using LayerId = uint32_t;
using PageId = uint32_t;

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
    // ICC profile associated with this color (optional, for color management)
    QString profileName;

    // Preflight rule types
    enum class PreflightRuleType {
        MissingFonts,
        LowResolutionImages,
        OutOfGamutColors,
        UnauthorizedColors,
        ObjectsOutsidePage,
        TransparencyIssues
    };

    struct PreflightRule {
        PreflightRuleType type;
        QString description;
        QString details;
        bool passed; // true = issue found, false = OK
    };

    Color() = default;
    static Color cmyk(float c, float m, float y, float k);
    static Color spot(const QString& name, float tint = 1.0f);
    static Color rgb(float r, float g, float b);
    static Color lab(float l, float a, float b);
    bool isValid() const { return type != Type::None; }
    
    bool operator==(const Color& other) const {
        return type == other.type &&
               components == other.components &&
               spotName == other.spotName &&
               qFuzzyCompare(tint, other.tint) &&
               profileName == other.profileName;
    }
    bool operator!=(const Color& other) const {
        return !(*this == other);
    }
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
    float fillAlpha = 1.0f;
    float strokeAlpha = 1.0f;
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
    DocumentObject(ObjectType type) : m_type(type), m_id(nextObjectId++), m_name("") {}
    virtual ~DocumentObject() = default;

    ObjectId id() const { return m_id; }
    ObjectType type() const { return m_type; }
    const QString& name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    void setId(ObjectId id) { m_id = id; }

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

    ObjectId softMaskId() const { return m_softMaskId; }
    void setSoftMaskId(ObjectId id) { m_softMaskId = id; }

    const std::vector<ObjectId>& clipMaskIds() const { return m_clipMasks; }
    void setClipMaskIds(const std::vector<ObjectId>& ids) { m_clipMasks = ids; }
    void addClipMaskId(ObjectId id) { m_clipMasks.push_back(id); }
    void clearClipMaskIds() { m_clipMasks.clear(); }

    const std::vector<ObjectId>& children() const { return m_children; }
    void addChild(ObjectId id) { m_children.push_back(id); }
    void removeChild(ObjectId id);

private:
    ObjectId m_id;
    static uint32_t nextObjectId;
    ObjectType m_type;
    QString m_name;
    bool m_visible = true;
    bool m_locked = false;
    QTransform m_transform;
    FillStroke m_fillStroke;
    LayerId m_layerId;
    ObjectId m_parentId;
    ObjectId m_softMaskId = 0;
    std::vector<ObjectId> m_clipMasks;
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

// Holds a clipping path imported from a PDF/vector operation (W / W*).
// Other objects reference this mask through DocumentObject::clipMaskIds().
class ClippingMaskObject : public DocumentObject {
public:
    ClippingMaskObject() : DocumentObject(ObjectType::ClippingMask) {}

    struct Element {
        enum class T { Move, Line, Cubic, Close } t;
        QPointF p1, p2, p3;
    };

    const std::vector<Element>& elements() const { return m_elements; }
    std::vector<Element>& elements() { return m_elements; }
    void addMove(const QPointF& p) { m_elements.push_back({Element::T::Move, p, {}, {}}); }
    void addLine(const QPointF& p) { m_elements.push_back({Element::T::Line, p, {}, {}}); }
    void addCurve(const QPointF& c1, const QPointF& c2, const QPointF& end) { m_elements.push_back({Element::T::Cubic, c1, c2, end}); }
    void closePath() { m_elements.push_back({Element::T::Close, {}, {}, {}}); }

    bool evenOdd() const { return m_evenOdd; }
    void setEvenOdd(bool e) { m_evenOdd = e; }

    QRectF boundingBox() const override;
    std::shared_ptr<DocumentObject> clone() const override;

private:
    std::vector<Element> m_elements;
    bool m_evenOdd = false;
};

// Soft mask (transparency mask) imported from PDF /SMask ExtGState.
// Holds the mask image (usually grayscale) and the /S mode (Alpha or Luminosity).
class OpacityMaskObject : public DocumentObject {
public:
    OpacityMaskObject() : DocumentObject(ObjectType::OpacityMask) {}

    const QImage& image() const { return m_image; }
    void setImage(const QImage& img) { m_image = img; }
    QString mode() const { return m_mode; }
    void setMode(const QString& m) { m_mode = m; }

    QRectF boundingBox() const override;
    std::shared_ptr<DocumentObject> clone() const override;

private:
    QImage m_image;
    QString m_mode = "Alpha";
};

class Layer {
public:
    Layer(const QString& name = "Layer") : m_name(name), m_id(nextLayerId++) {}

    uint32_t id() const { return m_id; }
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
    void insertObjectAt(size_t index, ObjectId id);
    void removeObject(ObjectId id);

private:
    uint32_t m_id;
    static uint32_t nextLayerId;
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
    Page() : m_id(nextId++) {}

    uint32_t id() const { return m_id; }
    const PageBoxes& boxes() const { return m_boxes; }
    PageBoxes& boxes() { return m_boxes; }

    const std::vector<uint32_t>& layers() const { return m_layers; }
    void addLayer(uint32_t id) { m_layers.push_back(id); }

    QRectF pageRect() const { return m_boxes.cropBox; }

private:
    uint32_t m_id;
    static uint32_t nextId;
    PageBoxes m_boxes;
    std::vector<uint32_t> m_layers;
};

class Document {
public:
    Document() = default;

    void addPage(std::shared_ptr<Page> page);
    void removePage(PageId id);
    std::shared_ptr<Page> page(PageId id);
    std::shared_ptr<Page> page(PageId id) const;
    const std::vector<PageId>& pageOrder() const { return m_pageOrder; }
    std::shared_ptr<Page> currentPage() const;

    void addLayer(std::shared_ptr<Layer> layer);
    void removeLayer(LayerId id);
    std::shared_ptr<Layer> layer(LayerId id);
    std::shared_ptr<Layer> layer(LayerId id) const;
    const std::vector<LayerId>& layerOrder() const { return m_layerOrder; }

    void addObject(std::shared_ptr<DocumentObject> obj);
    void removeObject(ObjectId id);
    std::shared_ptr<DocumentObject> object(ObjectId id);
    std::shared_ptr<DocumentObject> object(ObjectId id) const;
    std::vector<ObjectId> objectIds() const {
        std::vector<ObjectId> ids;
        ids.reserve(m_objects.size());
        for (const auto& [id, obj] : m_objects) ids.push_back(id);
        return ids;
    }
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