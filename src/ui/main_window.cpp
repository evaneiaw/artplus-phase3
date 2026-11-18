#include "main_window.h"
#include "../pdf/pdf_document.h"
#include "../pdf/pdf_importer.h"
#include "../pdf/pdf_exporter.h"
#include "../document/document_serializer.h"
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QScrollBar>
#include <QKeyEvent>
#include <QShowEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QFontMetrics>
#include <QLabel>
#include <unordered_set>
#include <cmath>
#include <limits>
#include <utility>

namespace ArtPlus {

// ==================================================== clip-aware graphics items
static QPainterPath painterPathFromClipMask(const std::vector<ClippingMaskObject::Element>& elems, bool evenOdd) {
    QPainterPath path;
    path.setFillRule(evenOdd ? Qt::OddEvenFill : Qt::WindingFill);
    QPointF current;
    bool started = false;
    for (const auto& e : elems) {
        switch (e.t) {
            case ClippingMaskObject::Element::T::Move:
                path.moveTo(e.p1);
                current = e.p1;
                started = true;
                break;
            case ClippingMaskObject::Element::T::Line:
                if (!started) { path.moveTo(e.p1); current = e.p1; started = true; }
                else { path.lineTo(e.p1); current = e.p1; }
                break;
            case ClippingMaskObject::Element::T::Cubic:
                if (!started) { path.moveTo(current); }
                else { path.cubicTo(e.p1, e.p2, e.p3); current = e.p3; }
                break;
            case ClippingMaskObject::Element::T::Close:
                path.closeSubpath();
                break;
        }
    }
    return path;
}

template <class Item>
class ClipAwareItem : public Item {
public:
    template <class... Args>
    explicit ClipAwareItem(Args&&... args) : Item(std::forward<Args>(args)...) {}
    void setClipPaths(const std::vector<QPainterPath>& paths) { m_clipPaths = paths; }
    void setSoftMaskImage(const QImage& mask) { m_mask = mask; }

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
        if (!m_mask.isNull()) {
            // Render the item into an offscreen image, multiply by the mask alpha,
            // then composite it onto the scene painter.
            QRectF r = Item::boundingRect();
            if (!r.isValid() || r.isEmpty()) {
                paintWithoutMask(painter, option, widget);
                return;
            }
            QImage img(r.width(), r.height(), QImage::Format_ARGB32);
            img.fill(Qt::transparent);
            QPainter off(&img);
            off.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
            off.translate(-r.left(), -r.top());
            paintWithoutMask(&off, option, widget);
            off.end();

            QImage mscaled = m_mask.scaled(img.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                 .convertToFormat(QImage::Format_Grayscale8);
            for (int y = 0; y < img.height(); ++y) {
                const uchar* mptr = mscaled.constScanLine(y);
                QRgb* dst = reinterpret_cast<QRgb*>(img.scanLine(y));
                for (int x = 0; x < img.width(); ++x) {
                    QRgb p = dst[x];
                    int alpha = (qAlpha(p) * mptr[x]) / 255;
                    dst[x] = qRgba(qRed(p), qGreen(p), qBlue(p), alpha);
                }
            }
            painter->drawImage(r.topLeft(), img);
            return;
        }
        paintWithoutMask(painter, option, widget);
    }

private:
    void paintWithoutMask(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        if (m_clipPaths.empty()) {
            Item::paint(painter, option, widget);
            return;
        }
        QPainterPath combined = m_clipPaths.front();
        for (size_t i = 1; i < m_clipPaths.size(); ++i) {
            combined = combined.intersected(m_clipPaths[i]);
        }
        painter->save();
        painter->setClipPath(combined, Qt::ReplaceClip);
        Item::paint(painter, option, widget);
        painter->restore();
    }

    std::vector<QPainterPath> m_clipPaths;
    QImage m_mask;
};

using ClipAwarePathItem = ClipAwareItem<QGraphicsPathItem>;
using ClipAwareTextItem = ClipAwareItem<QGraphicsSimpleTextItem>;
using ClipAwarePixmapItem = ClipAwareItem<QGraphicsPixmapItem>;

// Draws an image into the unit square (PDF image semantics: the surrounding
// cm/CTM maps 0..1 onto the intended rectangle).
class ImageUnitItem : public QGraphicsItem {
public:
    explicit ImageUnitItem(const QImage& img) : m_image(img) {}
    QRectF boundingRect() const override { return QRectF(0, 0, 1, 1); }
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->drawImage(QRectF(0, 0, 1, 1), m_image);
    }
private:
    QImage m_image;
};
using ClipAwareImageItem = ClipAwareItem<ImageUnitItem>;

// ================================================================== scene
void ArtPlusScene::setDocument(Document* doc) {
    m_doc = doc;
    // Choose the first available page as current
    if (m_doc && !m_doc->pageOrder().empty()) m_currentPage = m_doc->pageOrder().front();
    refresh();
}

void ArtPlusScene::setCurrentPage(PageId id) {
    m_currentPage = id;
    refresh();
}

void ArtPlusScene::refresh() {
    clear();
    if (!m_doc) return;

    // Pick the current page; fall back to the first page if unset/missing
    if (m_currentPage == 0 || m_doc->page(m_currentPage) == nullptr) {
        if (!m_doc->pageOrder().empty()) m_currentPage = m_doc->pageOrder().front();
    }
    auto page = m_doc->page(m_currentPage);

    // Background page
    if (page) {
        const auto& pageIds = m_doc->pageOrder();
        if (!pageIds.empty()) {
            QRectF pageRect(0, 0, page->boxes().mediaBox.width(), page->boxes().mediaBox.height());
            setSceneRect(pageRect.adjusted(-50, -50, 100, 100));
            // Page background (white paper)
            addRect(pageRect, QPen(Qt::black, 1), QBrush(Qt::white));
        }
    }

    // Only render objects that belong to the current page's layers
    std::unordered_set<LayerId> pageLayers;
    if (page) {
        for (LayerId lid : page->layers()) pageLayers.insert(lid);
    }

    if (!m_doc) return;

    // Build clip mask paths (scene coords) mapped into item-local coords
    auto clipPathsFor = [this](const std::shared_ptr<DocumentObject>& obj, const QTransform& itemTransform) -> std::vector<QPainterPath> {
        std::vector<QPainterPath> local;
        QTransform inv = itemTransform.inverted();
        for (ObjectId maskId : obj->clipMaskIds()) {
            auto m = m_doc->object(maskId);
            if (!m || m->type() != ObjectType::ClippingMask) continue;
            auto mask = std::dynamic_pointer_cast<ClippingMaskObject>(m);
            QPainterPath path = painterPathFromClipMask(mask->elements(), mask->evenOdd());
            local.push_back(inv.map(mask->transform().map(path)));
        }
        return local;
    };

    // Resolve soft mask image (alpha mask from the PDF /SMask ExtGState), if any
    auto softMaskFor = [this](const std::shared_ptr<DocumentObject>& obj) -> QImage {
        if (obj->softMaskId() == 0) return QImage();
        auto mask = m_doc->object(obj->softMaskId());
        if (!mask || mask->type() != ObjectType::OpacityMask) return QImage();
        if (auto mo = std::dynamic_pointer_cast<OpacityMaskObject>(mask)) return mo->image();
        return QImage();
    };

    for (const auto& [id, obj] : m_doc->objects()) {
        if (!pageLayers.empty() && !pageLayers.count(obj->layerId())) continue;
        if (obj->type() == ObjectType::ClippingMask) {
            // Draw the mask outline dashed so it is inspectable, but it is not a paintable object
            auto mask = std::dynamic_pointer_cast<ClippingMaskObject>(obj);
            if (mask) {
                QPainterPath path = painterPathFromClipMask(mask->elements(), mask->evenOdd());
                auto item = new QGraphicsPathItem(path);
                QPen dashPen(Qt::magenta, 1.0, Qt::DashLine);
                item->setPen(dashPen);
                item->setBrush(Qt::NoBrush);
                item->setData(Qt::UserRole, id);
                item->setVisible(obj->visible());
                addItem(item);
            }
            continue;
        }
        if (obj->type() == ObjectType::Path) {
            auto pathObj = std::dynamic_pointer_cast<PathObject>(obj);
            if (!pathObj) continue;
            QPainterPath painterPath;
            // Current subpath state
            QPointF currentSubpathStart;
            for (const auto& e : pathObj->elements()) {
                switch (e.type) {
                    case PathObject::PathElement::Type::Move:
                        painterPath.moveTo(e.p1);
                        currentSubpathStart = e.p1;
                        break;
                    case PathObject::PathElement::Type::Line:
                        painterPath.lineTo(e.p1);
                        break;
                    case PathObject::PathElement::Type::Curve:
                        if (painterPath.isEmpty()) painterPath.lineTo(e.p1);
                        else painterPath.cubicTo(e.p1, e.p2, e.p3);
                        break;
                    case PathObject::PathElement::Type::Close:
                        painterPath.closeSubpath();
                        break;
                }
            }

            Color fill = obj->fillStroke().fill;
            Color stroke = obj->fillStroke().stroke;
            bool hasFill = fill.isValid();
            bool hasStroke = stroke.isValid();

            auto toQColor = [](const Color& c) -> QColor {
                if (c.type == Color::Type::CMYK && c.components.size() >= 4) {
                    int c_i = std::clamp((int)std::round(c.components[0] * 100), 0, 100);
                    int m_i = std::clamp((int)std::round(c.components[1] * 100), 0, 100);
                    int y_i = std::clamp((int)std::round(c.components[2] * 100), 0, 100);
                    int k_i = std::clamp((int)std::round(c.components[3] * 100), 0, 100);
                    QColor qc(0, 0, 0);
                    qc.setCmykF(c.components[0], c.components[1], c.components[2], c.components[3], 1.0);
                    return qc;
                }
                if (c.type == Color::Type::RGB && c.components.size() >= 3) {
                    return QColor::fromRgbF(c.components[0], c.components[1], c.components[2]);
                }
                if (c.type == Color::Type::Gray && !c.components.isEmpty()) {
                    float v = c.components[0];
                    return QColor::fromRgbF(v, v, v);
                }
                if (c.type == Color::Type::Spot) {
                    // display as magenta-ish spot
                    return QColor(200, 0, 200);
                }
                return QColor(0, 0, 0);
            };

            QBrush brushFill = hasFill ? QBrush(toQColor(fill)) : QBrush(Qt::NoBrush);
            QPen penStroke;
            if (hasStroke) {
                penStroke.setColor(toQColor(stroke));
                penStroke.setWidthF(obj->fillStroke().strokeWidth);
                penStroke.setCapStyle(obj->fillStroke().capStyle);
                penStroke.setJoinStyle(obj->fillStroke().joinStyle);
                if (!obj->fillStroke().dashPattern.empty()) {
                    QVector<qreal> dash;
                    for (float v : obj->fillStroke().dashPattern) dash.push_back((qreal)v);
                    penStroke.setDashPattern(dash);
                }
            } else {
                penStroke.setStyle(Qt::NoPen);
            }

            QTransform objTransform = obj->transform();
            auto item = new ClipAwarePathItem(painterPath);
            item->setBrush(brushFill);
            item->setPen(hasStroke ? penStroke : QPen(Qt::NoPen));
            item->setTransform(objTransform);
            item->setClipPaths(clipPathsFor(obj, objTransform));
            item->setSoftMaskImage(softMaskFor(obj));
            item->setOpacity(hasFill ? obj->fillStroke().fillAlpha : obj->fillStroke().strokeAlpha);
            item->setData(Qt::UserRole, id); // ObjectId (uint32)
            if (obj->visible()) item->setVisible(true);
            addItem(item);
        }
        else if (obj->type() == ObjectType::Text) {
            auto textObj = std::dynamic_pointer_cast<TextObject>(obj);
            if (!textObj) continue;
            auto item = new ClipAwareTextItem(textObj->text());
            QFont font(textObj->fontFamily(), (int)std::round(textObj->fontSize()));
            font.setBold(textObj->bold());
            font.setItalic(textObj->italic());
            item->setFont(font);
            // fill color
            Color fill = obj->fillStroke().fill;
            QColor qfill = fill.isValid()
                ? (fill.type == Color::Type::CMYK && fill.components.size() >= 4
                    ? QColor::fromRgbF((1 - fill.components[0]) * (1 - fill.components[3]),
                                       (1 - fill.components[1]) * (1 - fill.components[3]),
                                       (1 - fill.components[2]) * (1 - fill.components[3]))
                    : fill.type == Color::Type::RGB && fill.components.size() >= 3
                        ? QColor::fromRgbF(fill.components[0], fill.components[1], fill.components[2])
                        : fill.type == Color::Type::Gray && !fill.components.isEmpty()
                            ? QColor::fromRgbF(fill.components[0], fill.components[0], fill.components[0])
                            : QColor(200, 0, 200))
                : QColor(0, 0, 0);
            item->setBrush(qfill);
            // position from transform, but draw unflipped to avoid mirrored text
            QTransform t = textObj->transform();
            QPointF org = t.map(QPointF(0, 0));
            item->setPos(org);
            double rotationDeg = (t.m11() != 0 || t.m12() != 0) ? std::atan2(t.m12(), t.m11()) * 180.0 / M_PI : 0.0;
            item->setRotation(rotationDeg);
            // effective item matrix used for local clip mapping
            QTransform effective;
            effective.translate(org.x(), org.y());
            if (rotationDeg != 0.0) effective.rotate(rotationDeg);
            item->setClipPaths(clipPathsFor(obj, effective));
            item->setSoftMaskImage(softMaskFor(obj));
            item->setOpacity(obj->fillStroke().fillAlpha);
            item->setData(Qt::UserRole, id);
            item->setVisible(obj->visible());
            addItem(item);
        }
        else if (obj->type() == ObjectType::Image) {
            auto imgObj = std::dynamic_pointer_cast<ImageObject>(obj);
            if (!imgObj) continue;
            auto item = new ClipAwareImageItem(imgObj->image());
            QTransform objTransform = obj->transform();
            item->setTransform(objTransform);
            item->setClipPaths(clipPathsFor(obj, objTransform));
            item->setSoftMaskImage(softMaskFor(obj));
            item->setOpacity(obj->fillStroke().fillAlpha);
            item->setData(Qt::UserRole, id);
            item->setVisible(obj->visible());
            addItem(item);
        }
    }
}

void ArtPlusScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        bool additive = (event->modifiers() & Qt::ShiftModifier) || (event->modifiers() & Qt::ControlModifier);
        const auto ids = hitTest(event->scenePos());
        if (!ids.empty()) {
            selectIds(ids, additive);
            m_dragging = true;
            m_dragStart = event->scenePos();
            m_dragStartTransforms.clear();
            if (m_doc) {
                for (ObjectId id : selectedObjectIds()) {
                    auto obj = m_doc->object(id);
                    if (obj) m_dragStartTransforms[id] = obj->transform();
                }
            }
            return;
        }
        // Start rubber band (also starts empty marquee)
        m_rubberBand = true;
        m_additive = additive;
        m_rubberBandRect = QRectF(event->scenePos(), QSizeF(0, 0));
        if (!additive) clearSelection();
        return;
    }
    QGraphicsScene::mousePressEvent(event);
}

void ArtPlusScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_dragging) {
        QPointF delta = event->scenePos() - m_dragStart;
        const auto ids = selectedObjectIds();
        for (ObjectId id : ids) {
            auto obj = m_doc->object(id);
            if (!obj) continue;
            QTransform t = obj->transform();
            obj->setTransform(QTransform(t.m11(), t.m12(), t.m21(), t.m22(), t.m31() + delta.x(), t.m32() + delta.y()));
        }
        m_dragStart = event->scenePos();
        m_doc->setModified(true);
        emit documentModified();
        refresh();
        emit selectionChangedObjects(selectedObjectIds());
        return;
    }
    if (m_rubberBand) {
        m_rubberBandRect = QRectF(m_rubberBandRect.topLeft(), event->scenePos()).normalized();
        // visual feedback via rubber band drawn in paintEvent? we skip; selection happens on release
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void ArtPlusScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_dragging) {
        m_dragging = false;
        if (m_doc && m_undoStack && !m_dragStartTransforms.empty()) {
            std::vector<ObjectId> moved;
            std::vector<QTransform> before, after;
            moved.reserve(m_dragStartTransforms.size());
            bool anyChanged = false;
            for (const auto& [id, beforeT] : m_dragStartTransforms) {
                auto obj = m_doc->object(id);
                if (!obj) continue;
                moved.push_back(id);
                before.push_back(beforeT);
                after.push_back(obj->transform());
                if (obj->transform() != beforeT) anyChanged = true;
            }
            if (anyChanged) {
                auto cmd = std::make_unique<TransformCommand>(moved, before, after,
                    QString("Move %1 object(s)").arg(moved.size()));
                m_undoStack->push(std::move(cmd));
                m_doc->setModified(true);
                emit documentModified();
            }
            m_dragStartTransforms.clear();
        }
        return;
    }
    if (m_rubberBand) {
        m_rubberBand = false;
        std::vector<ObjectId> ids;
        for (auto item : QGraphicsScene::items()) {
            QRectF r = item->sceneBoundingRect();
            if (m_rubberBandRect.intersects(r)) {
                bool ok = false;
                quint32 id = item->data(Qt::UserRole).toUInt(&ok);
                if (ok) ids.push_back((ObjectId)id);
            }
        }
        if (m_additive) {
            auto cur = selectedObjectIds();
            for (ObjectId id : ids) if (std::find(cur.begin(), cur.end(), id) == cur.end()) cur.push_back(id);
            ids = cur;
        }
        selectIds(ids, false);
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void ArtPlusScene::setSelectionIds(const std::vector<ObjectId>& ids) {
    selectIds(ids, false);
}

void ArtPlusScene::selectIds(const std::vector<ObjectId>& ids, bool additive) {
    std::vector<ObjectId> current;
    if (additive) current = selectedObjectIds();

    for (auto item : items()) {
        bool ok = false;
        quint32 id = item->data(Qt::UserRole).toUInt(&ok);
        if (!ok) continue;
        bool wanted = std::find(ids.begin(), ids.end(), (ObjectId)id) != ids.end();
        if (additive) {
            wanted = wanted || std::find(current.begin(), current.end(), (ObjectId)id) != current.end();
        }
        item->setSelected(wanted);
    }
    if (m_doc) {
        m_doc->clearSelection();
        for (ObjectId id : ids) m_doc->selectObject(id, true);
    }
    emit selectionChangedObjects(ids);
}

std::vector<ObjectId> ArtPlusScene::selectedObjectIds() const {
    std::vector<ObjectId> ids;
    for (auto item : items()) {
        if (!item->isSelected()) continue;
        bool ok = false;
        quint32 id = item->data(Qt::UserRole).toUInt(&ok);
        if (ok) ids.push_back((ObjectId)id);
    }
    return ids;
}

std::vector<ObjectId> ArtPlusScene::hitTest(const QPointF& scenePos) const {
    std::vector<ObjectId> ids;
    for (auto item : QGraphicsScene::items(scenePos)) {
        bool ok = false;
        quint32 id = item->data(Qt::UserRole).toUInt(&ok);
        if (ok && !std::count(ids.begin(), ids.end(), (ObjectId)id)) ids.push_back((ObjectId)id);
    }
    return ids;
}

void ArtPlusScene::deleteSelected() {
    if (!m_doc) return;
    const auto ids = selectedObjectIds();
    if (ids.empty()) return;

    // Capture snapshot for undo BEFORE mutating
    if (m_undoStack) {
        auto cmd = DeleteObjectsCommand::capture(*m_doc, ids);
        m_undoStack->push(std::move(cmd));
    }
    for (ObjectId id : ids) {
        m_doc->removeObject(id);
        // remove from layers too
        for (LayerId lid : m_doc->layerOrder()) {
            auto layer = m_doc->layer(lid);
            if (layer) layer->removeObject(id);
        }
    }
    m_doc->clearSelection();
    m_doc->setModified(true);
    emit documentModified();
    refresh();
    emit selectionChangedObjects({});
}

// ================================================================== view
ArtPlusView::ArtPlusView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent) {
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setMouseTracking(true);
    setDragMode(QGraphicsView::NoDrag);
}

void ArtPlusView::setTool(int tool) {
    m_tool = tool;
    // Cursors
    if (tool == 1) setCursor(Qt::OpenHandCursor);
    else if (tool == 2) setCursor(Qt::SizeHorCursor);
    else setCursor(Qt::ArrowCursor);
}

void ArtPlusView::mousePressEvent(QMouseEvent* event) {
    if (m_tool == 1 && event->button() == Qt::LeftButton) { // hand tool
        m_panning = true;
        m_panStart = event->pos();
        m_panStartScroll = QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (m_tool == 2 && event->button() == Qt::LeftButton) { // zoom tool
        zoomAt(event->pos(), event->modifiers() & Qt::AltModifier ? 0.8 : 1.25);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void ArtPlusView::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        QPoint delta = m_panStart - event->pos();
        horizontalScrollBar()->setValue(m_panStartScroll.x() + delta.x());
        verticalScrollBar()->setValue(m_panStartScroll.y() + delta.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void ArtPlusView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_panning && event->button() == Qt::LeftButton) {
        m_panning = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void ArtPlusView::wheelEvent(QWheelEvent* event) {
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    zoomAt(event->position().toPoint(), factor);
    event->accept();
}

void ArtPlusView::zoomAt(const QPoint& pos, double factor) {
    QPointF scenePos = mapToScene(pos);
    scale(factor, factor);
    centerOn(scenePos);
    emit zoomChanged(transform().m11());
}

// ================================================================== window
ArtPlusMainWindow::ArtPlusMainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("ArtPlus");
    resize(1400, 900);
    setupUi();
    setupMenusAndToolbars();
}

void ArtPlusMainWindow::setupUi() {
    m_scene = new ArtPlusScene(this);
    m_scene->setUndoStack(&m_history);
    connect(m_scene, &ArtPlusScene::selectionChangedObjects, this, &ArtPlusMainWindow::onSelectionChanged);
    connect(m_scene, &ArtPlusScene::documentModified, this, &ArtPlusMainWindow::updateModifiedIndicator);
    m_view = new ArtPlusView(m_scene, this);
    setCentralWidget(m_view);

    m_statusBar = statusBar();
    m_statusBar->showMessage("Ready");

    // Layers dock
    m_layersDock = new QDockWidget("Layers", this);
    m_layersDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_layersList = new QListWidget();
    m_layersDock->setWidget(m_layersList);
    addDockWidget(Qt::RightDockWidgetArea, m_layersDock);

    // Objects dock (tree)
    m_objectsDock = new QDockWidget("Objects", this);
    m_objectsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_objectTree = new QTreeWidget();
    m_objectTree->setHeaderLabels({"Name", "Type"});
    m_objectsDock->setWidget(m_objectTree);
    addDockWidget(Qt::RightDockWidgetArea, m_objectsDock);
}

void ArtPlusMainWindow::setupMenusAndToolbars() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* openPdfAct = fileMenu->addAction("&Open PDF...");
    openPdfAct->setShortcut(QKeySequence::Open);
    connect(openPdfAct, &QAction::triggered, this, &ArtPlusMainWindow::openPdf);

    QAction* openAct = fileMenu->addAction("Open .&artplus...");
    connect(openAct, &QAction::triggered, this, &ArtPlusMainWindow::openArtPlus);

    fileMenu->addSeparator();
    QAction* saveAct = fileMenu->addAction("&Save .artplus");
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &ArtPlusMainWindow::saveArtPlus);

    QAction* exportPdfAct = fileMenu->addAction("Export PDF...");
    exportPdfAct->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportPdfAct, &QAction::triggered, this, &ArtPlusMainWindow::exportPdf);

    fileMenu->addSeparator();
    QAction* exitAct = fileMenu->addAction("E&xit");
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // Edit menu
    QMenu* editMenu = menuBar()->addMenu("&Edit");
    QAction* undoAct = editMenu->addAction("&Undo");
    undoAct->setShortcut(QKeySequence::Undo);
    connect(undoAct, &QAction::triggered, this, &ArtPlusMainWindow::undoAction);
    QAction* redoAct = editMenu->addAction("&Redo");
    redoAct->setShortcut(QKeySequence::Redo);
    connect(redoAct, &QAction::triggered, this, &ArtPlusMainWindow::redoAction);
    editMenu->addSeparator();
    QAction* selectAllAct = editMenu->addAction("Select &All");
    selectAllAct->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAct, &QAction::triggered, this, &ArtPlusMainWindow::selectAll);
    QAction* deselectAllAct = editMenu->addAction("Deselect &All");
    connect(deselectAllAct, &QAction::triggered, this, &ArtPlusMainWindow::deselectAll);
    editMenu->addSeparator();
    QAction* deleteAct = editMenu->addAction("&Delete Selected");
    deleteAct->setShortcut(QKeySequence::Delete);
    connect(deleteAct, &QAction::triggered, this, &ArtPlusMainWindow::deleteSelected);
    QAction* stepRepeatAct = editMenu->addAction("Step & Repeat...");
    stepRepeatAct->setShortcut(QKeySequence("S"));
    connect(stepRepeatAct, &QAction::triggered, this, &ArtPlusMainWindow::stepRepeat);
    QAction* impositionAct = editMenu->addAction("Imposition...");
    impositionAct->setShortcut(QKeySequence("I"));
    connect(impositionAct, &QAction::triggered, this, &ArtPlusMainWindow::imposition);

    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");
    QAction* zoomInAct = viewMenu->addAction("Zoom &In");
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAct, &QAction::triggered, this, &ArtPlusMainWindow::zoomIn);
    QAction* zoomOutAct = viewMenu->addAction("Zoom &Out");
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAct, &QAction::triggered, this, &ArtPlusMainWindow::zoomOut);
    QAction* zoomFitAct = viewMenu->addAction("Zoom to &Fit");
    zoomFitAct->setShortcut(QKeySequence("Ctrl+0"));
    connect(zoomFitAct, &QAction::triggered, this, &ArtPlusMainWindow::zoomFit);

    // Toolbar
    QToolBar* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    auto addTool = [&](const QString& text, const QString& tip, auto slot) {
        QAction* act = toolbar->addAction(text);
        act->setToolTip(tip);
        connect(act, &QAction::triggered, this, slot);
        return act;
    };

    addTool("Open PDF", "Open a PDF file and import its content", &ArtPlusMainWindow::openPdf);
    addTool("Open .artplus", "Open an ArtPlus document", &ArtPlusMainWindow::openArtPlus);
    addTool("Save", "Save as .artplus", &ArtPlusMainWindow::saveArtPlus);
    addTool("Export PDF", "Export the document to PDF", &ArtPlusMainWindow::exportPdf);
    toolbar->addSeparator();

    toolbar->addWidget(new QLabel("Page:", toolbar));
    m_pageCombo = new QComboBox(toolbar);
    m_pageCombo->setToolTip("Choose the page to view");
    connect(m_pageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ArtPlusMainWindow::onPageChanged);
    toolbar->addWidget(m_pageCombo);
    toolbar->addSeparator();

    addTool("Select", "Selection tool (V)", &ArtPlusMainWindow::selectTool);
    addTool("Pan", "Pan/Hand tool (H)", &ArtPlusMainWindow::handTool);
    addTool("Zoom", "Zoom tool (Z)", &ArtPlusMainWindow::zoomTool);
    toolbar->addSeparator();

    addTool("Select All", "Select all objects (Ctrl+A)", &ArtPlusMainWindow::selectAll);
    addTool("Deselect", "Deselect all", &ArtPlusMainWindow::deselectAll);
    addTool("Delete", "Delete selected objects (Del)", &ArtPlusMainWindow::deleteSelected);
    addTool("Step & Repeat", "Step & Repeat selected objects in a grid (S)", &ArtPlusMainWindow::stepRepeat);
    addTool("Imposition", "Imposition layout (2-up, 4-up)", &ArtPlusMainWindow::imposition);
    toolbar->addSeparator();

    addTool("Zoom In", "Zoom in (Ctrl++)", &ArtPlusMainWindow::zoomIn);
    addTool("Zoom Out", "Zoom out (Ctrl+-)", &ArtPlusMainWindow::zoomOut);
    addTool("Fit", "Fit page in window (Ctrl+0)", &ArtPlusMainWindow::zoomFit);
}

void ArtPlusMainWindow::viewPage(int index) {
    if (m_pageCombo && index >= 0 && index < m_pageCombo->count()) {
        m_pageCombo->setCurrentIndex(index); // emits currentIndexChanged -> onPageChanged
    }
}

bool ArtPlusMainWindow::openPdfPath(const QString& path) {
    PDF::PdfDocument pdf;
    if (!pdf.open(path)) {
        return false;
    }
    m_document = Document();
    PdfImportResult result = PdfImporter::import(pdf, m_document);
    if (!result.ok) return false;
    m_originalPdfPath = path;
    m_document.setFilePath(path);
    m_document.setModified(false);
    m_history.clear();
    rebuildScene();
    updateDocumentInfo();
    return true;
}

void ArtPlusMainWindow::openPdf() {
    QString path = QFileDialog::getOpenFileName(this, "Open PDF", QString(), "PDF Files (*.pdf)");
    if (path.isEmpty()) return;
    openPdfPath(path);
    m_statusBar->showMessage(QString("Imported %1").arg(QFileInfo(path).fileName()));
}

void ArtPlusMainWindow::openArtPlus() {
    QString path = QFileDialog::getOpenFileName(this, "Open .artplus", QString(), "ArtPlus Files (*.artplus *.json)");
    if (path.isEmpty()) return;

    Document doc;
    if (!DocumentSerializer::load(path, doc)) {
        QMessageBox::critical(this, "Open failed", "Could not load: " + path);
        return;
    }
    m_document = doc;
    m_originalPdfPath.clear();
    m_history.clear();
    rebuildScene();
    updateDocumentInfo();
    m_statusBar->showMessage("Opened " + QFileInfo(path).fileName());
}

void ArtPlusMainWindow::saveArtPlus() {
    QString path;
    if (m_document.filePath().isEmpty() || !m_document.filePath().endsWith(".artplus")) {
        path = QFileDialog::getSaveFileName(this, "Save .artplus", QString(), "ArtPlus Files (*.artplus)");
        if (path.isEmpty()) return;
        if (!path.endsWith(".artplus")) path += ".artplus";
    } else {
        path = m_document.filePath();
    }

    m_document.setFilePath(path);
    if (!DocumentSerializer::save(m_document, path)) {
        QMessageBox::critical(this, "Save failed", "Could not save: " + path);
        return;
    }
    m_document.setModified(false);
    m_history.clear();
    updateModifiedIndicator();
    m_statusBar->showMessage("Saved " + QFileInfo(path).fileName());
}

void ArtPlusMainWindow::exportPdf() {
    QString path = QFileDialog::getSaveFileName(this, "Export PDF", QString(), "PDF Files (*.pdf)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".pdf")) path += ".pdf";

    if (!PDF::PdfExporter::exportPdf(m_document, path, m_originalPdfPath)) {
        QMessageBox::critical(this, "Export failed", "Could not export to PDF: " + path);
        return;
    }
    m_statusBar->showMessage("Exported " + QFileInfo(path).fileName());
}

void ArtPlusMainWindow::zoomIn() { m_view->scale(1.25, 1.25); m_statusBar->showMessage(QString("Zoom %1%").arg((int)(m_view->transform().m11() * 100))); }
void ArtPlusMainWindow::zoomOut() { m_view->scale(0.8, 0.8); m_statusBar->showMessage(QString("Zoom %1%").arg((int)(m_view->transform().m11() * 100))); }
void ArtPlusMainWindow::zoomFit() {
    if (!m_scene->hasItems()) return;
    QRectF rect = m_scene->itemsBoundingRect();
    m_view->fitInView(rect, Qt::KeepAspectRatio);
    m_statusBar->showMessage(QString("Zoom %1%").arg((int)(m_view->transform().m11() * 100)));
}

void ArtPlusMainWindow::undoAction() {
    if (!m_history.canUndo()) {
        m_statusBar->showMessage("Nothing to undo");
        return;
    }
    QString what = m_history.undoDescription();
    m_history.undo(m_document);
    m_scene->refresh();
    m_document.setModified(true);
    updateModifiedIndicator();
    m_statusBar->showMessage(QString("Undid: %1").arg(what));
}

void ArtPlusMainWindow::redoAction() {
    if (!m_history.canRedo()) {
        m_statusBar->showMessage("Nothing to redo");
        return;
    }
    QString what = m_history.redoDescription();
    m_history.redo(m_document);
    m_scene->refresh();
    m_document.setModified(true);
    updateModifiedIndicator();
    m_statusBar->showMessage(QString("Redid: %1").arg(what));
}

void ArtPlusMainWindow::selectTool() { m_view->setTool(0); }
void ArtPlusMainWindow::handTool() { m_view->setTool(1); }
void ArtPlusMainWindow::zoomTool() { m_view->setTool(2); }

void ArtPlusMainWindow::selectAll() {
    std::vector<ObjectId> ids;
    for (auto id : m_document.objectIds()) ids.push_back(id);
    m_scene->setSelectionIds(ids);
}

void ArtPlusMainWindow::deselectAll() {
    m_scene->setSelectionIds({});
}

void ArtPlusMainWindow::deleteSelected() {
    m_scene->deleteSelected();
    updateDocumentInfo();
}

void ArtPlusMainWindow::stepRepeat() {
    // Step & Repeat: duplicate selected objects in a grid
    const auto selectedIds = m_scene->selectedObjectIds();
    if (selectedIds.empty()) {
        m_statusBar->showMessage("No objects selected for step repeat");
        return;
    }

    // Get transform parameters from user via simple input dialogs
    bool ok;
    int copiesX = QInputDialog::getInt(this, "Step & Repeat", "Copies in X direction:", 3, 1, 100, 1, &ok);
    if (!ok) return;
    int copiesY = QInputDialog::getInt(this, "Step & Repeat", "Copies in Y direction:", 3, 1, 100, 1, &ok);
    if (!ok) return;
    qreal spacingX = QInputDialog::getDouble(this, "Step & Repeat", "Horizontal spacing:", 50.0, 0, 10000, 0.1, &ok);
    if (!ok) return;
    qreal spacingY = QInputDialog::getDouble(this, "Step & Repeat", "Vertical spacing:", 50.0, 0, 10000, 0.1, &ok);
    if (!ok) return;
    qreal angle = QInputDialog::getDouble(this, "Step & Repeat", "Rotation angle (degrees):", 0.0, -360, 360, 1.0, &ok);
    if (!ok) return;

    if (!m_document.isValid()) return;

    // Capture snapshot for undo
    if (m_history.canUndo()) {
        // Create a transform command for step & repeat
        auto cmd = std::make_unique<TransformCommand>(selectedIds, selectedIds, selectedIds,
            QString("Step & Repeat: %1x%2 grid").arg(copiesX).arg(copiesY));
        m_history.push(std::move(cmd));
    }

    // Duplicate objects in grid
    QTransform rotateTransform;
    if (angle != 0.0) {
        rotateTransform.rotate(angle);
    }

    for (int i = 0; i < copiesX; ++i) {
        for (int j = 0; j < copiesY; ++j) {
            // Calculate position: i * spacingX horizontally, j * spacingY vertically
            QTransform translateTransform;
            translateTransform.translate(i * spacingX, j * spacingY);

            // Apply to each selected object
            for (ObjectId id : selectedIds) {
                auto obj = m_document.object(id);
                if (!obj) continue;

                QTransform originalTransform = obj->transform();
                QTransform newTransform = originalTransform * translateTransform * rotateTransform;

                auto copy = obj->clone();
                copy->setTransform(newTransform);
                copy->setLayerId(obj->layerId());
                copy->setClipMaskIds(obj->clipMaskIds());
                copy->setSoftMaskId(obj->softMaskId());
                copy->setVisible(obj->visible());
                // Generate a new ID for the copy
                copy->setId(m_document.nextObjectId++);
                m_document.addObject(copy);

                // Add to selection
                m_document.selectObject(copy->id(), true);
            }
        }
    }

    m_document.setModified(true);
    rebuildScene();
    emit selectionChangedObjects(m_document.selectedObjectIds());
    m_statusBar->showMessage(QString("Step & Repeat completed: %1x%2 grid").arg(copiesX).arg(copiesY));
}

void ArtPlusMainWindow::imposition() {
    // Imposition: arrange pages for printing and folding
    if (!m_document.isValid()) return;

    bool ok;
    int type = QInputDialog::getItem(this, "Imposition", "Imposition type:",
        {"None", "Half (2-up)", "Quarter (4-up)"}, 0, &ok);
    if (!ok) return;

    if (type == 0) { // None
        m_statusBar->showMessage("Imposition cancelled");
        return;
    }

    // Get the page order
    auto pageOrder = m_document.pageOrder();
    if (pageOrder.size() < 2) {
        m_statusBar->showMessage("Need at least 2 pages for imposition");
        return;
    }

    // Calculate imposition layout
    int numPages = (int)pageOrder.size();
    int sheetsNeeded = 1;
    while (sheetsNeeded * 4 < numPages) sheetsNeeded++;

    // For now, just show a message with the imposition info
    // Full imposition would rearrange page order for proper signature folding
    QString message;
    if (type == 1) { // Half
        message = QString("Imposition Half (2-up): %1 pages arranged on %2 sheets")
            .arg(numPages).arg(sheetsNeeded * 2);
    } else { // Quarter
        message = QString("Imposition Quarter (4-up): %1 pages arranged on %2 sheets")
            .arg(numPages).arg(sheetsNeeded);
    }

    m_statusBar->showMessage(message);
    // TODO: Full imposition implementation - rearrange pages for proper signature folding
    // - 2-up: pages arranged front/back on each sheet
    // - 4-up: pages arranged in signature order for folding
    // - Perfect binding: front/back ordering ensures correct reading order after folding
}

void ArtPlusMainWindow::onSelectionChanged(const std::vector<ObjectId>& ids) {
    m_objectTree->clear();
    for (ObjectId id : ids) {
        auto obj = m_document.object(id);
        if (obj) {
            auto item = new QTreeWidgetItem({obj->name(), obj->type() == ObjectType::Path ? "Path" : obj->type() == ObjectType::Text ? "Text" : obj->type() == ObjectType::Image ? "Image" : "Other"});
            m_objectTree->addTopLevelItem(item);
        }
    }
    if (!ids.empty()) m_statusBar->showMessage(QString("%1 object(s) selected").arg(ids.size()));
}

void ArtPlusMainWindow::updateModifiedIndicator() {
    if (m_document.modified()) {
        m_statusBar->showMessage("Modified");
    }
}

void ArtPlusMainWindow::rebuildScene() {
    m_scene->setDocument(&m_document);
    updatePageCombo();

    // Layers list shows the current page's layers
    m_layersList->clear();
    if (!m_pageIds.empty()) {
        if (auto page = m_document.page(m_pageIds.front())) {
            for (LayerId lid : page->layers()) {
                auto layer = m_document.layer(lid);
                if (layer) m_layersList->addItem(layer->name() + (layer->visible() ? "" : " (hidden)"));
            }
        }
    } else {
        for (LayerId lid : m_document.layerOrder()) {
            auto layer = m_document.layer(lid);
            if (layer) m_layersList->addItem(layer->name() + (layer->visible() ? "" : " (hidden)"));
        }
    }
    m_objectTree->clear();
}

void ArtPlusMainWindow::updatePageCombo() {
    m_pageIds.clear();
    if (!m_pageCombo) return;
    m_pageCombo->blockSignals(true);
    m_pageCombo->clear();
    int i = 0;
    for (PageId pid : m_document.pageOrder()) {
        m_pageIds.push_back(pid);
        m_pageCombo->addItem(QString("Page %1").arg(i + 1));
        ++i;
    }
    if (!m_pageIds.empty()) m_pageCombo->setCurrentIndex(0);
    m_pageCombo->blockSignals(false);
}

void ArtPlusMainWindow::onPageChanged(int index) {
    if (index < 0 || index >= (int)m_pageIds.size()) return;
    m_scene->setCurrentPage(m_pageIds[index]);
    // update layers dock to reflect the current page
    m_layersList->clear();
    if (auto page = m_document.page(m_pageIds[index])) {
        for (LayerId lid : page->layers()) {
            auto layer = m_document.layer(lid);
            if (layer) m_layersList->addItem(layer->name() + (layer->visible() ? "" : " (hidden)"));
        }
    }
    m_statusBar->showMessage(QString("Viewing page %1 of %2").arg(index + 1).arg(m_pageIds.size()));
}

void ArtPlusMainWindow::updateDocumentInfo() {
    if (m_document.pageOrder().empty()) {
        m_statusBar->showMessage("Ready");
        return;
    }
}

} // namespace ArtPlus
