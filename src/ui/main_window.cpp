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
#include <QFontMetrics>
#include <cmath>
#include <limits>

namespace ArtPlus {

// ================================================================== scene
void ArtPlusScene::setDocument(Document* doc) {
    m_doc = doc;
    refresh();
}

void ArtPlusScene::refresh() {
    clear();
    if (!m_doc) return;

    // Background page
    const auto& pageIds = m_doc->pageOrder();
    if (!pageIds.empty()) {
        auto page = m_doc->page(pageIds.front());
        if (page) {
            QRectF pageRect(0, 0, page->boxes().mediaBox.width(), page->boxes().mediaBox.height());
            setSceneRect(pageRect.adjusted(-50, -50, 100, 100));
            // Page background (white paper)
            addRect(pageRect, QPen(Qt::black, 1), QBrush(Qt::white));
        }
    }

    if (!m_doc) return;

    for (const auto& [id, obj] : m_doc->objects()) {
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

            QGraphicsPathItem* item = new QGraphicsPathItem(painterPath);
            item->setBrush(brushFill);
            item->setPen(hasStroke ? penStroke : QPen(Qt::NoPen));
            item->setTransform(obj->transform());
            item->setData(Qt::UserRole, id); // ObjectId (uint32)
            if (obj->visible()) item->setVisible(true);
            addItem(item);
        }
        else if (obj->type() == ObjectType::Text) {
            auto textObj = std::dynamic_pointer_cast<TextObject>(obj);
            if (!textObj) continue;
            auto item = new QGraphicsSimpleTextItem(textObj->text());
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
            item->setPos(t.map(QPointF(0, 0)));
            item->setRotation(t.m11() != 0 ? std::atan2(t.m12(), t.m11()) * 180.0 / M_PI : 0.0);
            item->setData(Qt::UserRole, id);
            item->setVisible(obj->visible());
            addItem(item);
        }
        else if (obj->type() == ObjectType::Image) {
            auto imgObj = std::dynamic_pointer_cast<ImageObject>(obj);
            if (!imgObj) continue;
            QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(imgObj->image()));
            item->setTransform(obj->transform());
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
    if (m_dragging) { m_dragging = false; return; }
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
    for (ObjectId id : selectedObjectIds()) {
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
    QAction* selectAllAct = editMenu->addAction("Select &All");
    selectAllAct->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAct, &QAction::triggered, this, &ArtPlusMainWindow::selectAll);
    QAction* deselectAllAct = editMenu->addAction("Deselect &All");
    connect(deselectAllAct, &QAction::triggered, this, &ArtPlusMainWindow::deselectAll);
    editMenu->addSeparator();
    QAction* deleteAct = editMenu->addAction("&Delete Selected");
    deleteAct->setShortcut(QKeySequence::Delete);
    connect(deleteAct, &QAction::triggered, this, &ArtPlusMainWindow::deleteSelected);

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

    addTool("Select", "Selection tool (V)", &ArtPlusMainWindow::selectTool);
    addTool("Pan", "Pan/Hand tool (H)", &ArtPlusMainWindow::handTool);
    addTool("Zoom", "Zoom tool (Z)", &ArtPlusMainWindow::zoomTool);
    toolbar->addSeparator();

    addTool("Select All", "Select all objects (Ctrl+A)", &ArtPlusMainWindow::selectAll);
    addTool("Deselect", "Deselect all", &ArtPlusMainWindow::deselectAll);
    addTool("Delete", "Delete selected objects (Del)", &ArtPlusMainWindow::deleteSelected);
    toolbar->addSeparator();

    addTool("Zoom In", "Zoom in (Ctrl++)", &ArtPlusMainWindow::zoomIn);
    addTool("Zoom Out", "Zoom out (Ctrl+-)", &ArtPlusMainWindow::zoomOut);
    addTool("Fit", "Fit page in window (Ctrl+0)", &ArtPlusMainWindow::zoomFit);
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
    m_layersList->clear();
    for (LayerId lid : m_document.layerOrder()) {
        auto layer = m_document.layer(lid);
        if (layer) m_layersList->addItem(layer->name() + (layer->visible() ? "" : " (hidden)"));
    }
    m_objectTree->clear();
}

void ArtPlusMainWindow::updateDocumentInfo() {
    if (m_document.pageOrder().empty()) {
        m_statusBar->showMessage("Ready");
        return;
    }
}

} // namespace ArtPlus
