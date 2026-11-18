#pragma once

#include "../document/document_model.h"
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QToolBar>
#include <QStatusBar>
#include <QListWidget>
#include <QDockWidget>
#include <QTreeWidget>
#include <QMenuBar>
#include <QComboBox>
#include <memory>
#include <vector>

namespace ArtPlus {

// ---------------------------------------------------------------- scene
class ArtPlusScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit ArtPlusScene(QObject* parent = nullptr) : QGraphicsScene(parent) {}
    void setDocument(Document* doc);
    Document* document() const { return m_doc; }
    void refresh();
    bool hasItems() const { return !QGraphicsScene::items().isEmpty(); }
    void setCurrentPage(PageId id);
    PageId currentPage() const { return m_currentPage; }

    // Selection helpers
    std::vector<ObjectId> selectedObjectIds() const;
    void setSelectionIds(const std::vector<ObjectId>& ids);
    void deleteSelected();

signals:
    void selectionChangedObjects(const std::vector<ObjectId>& ids);
    void documentModified();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    Document* m_doc = nullptr;
    PageId m_currentPage = 0;
    bool m_dragging = false;
    QPointF m_dragStart;
    bool m_rubberBand = false;
    QRectF m_rubberBandRect;
    bool m_additive = false;

    std::vector<ObjectId> hitTest(const QPointF& scenePos) const;
    void selectIds(const std::vector<ObjectId>& ids, bool additive);
};

// ---------------------------------------------------------------- view
class ArtPlusView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ArtPlusView(QGraphicsScene* scene, QWidget* parent = nullptr);
    void setTool(int tool); // 0 = select, 1 = hand, 2 = zoom
    int tool() const { return m_tool; }

signals:
    void zoomChanged(double factor);
    void deleteRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    int m_tool = 0;
    bool m_panning = false;
    QPoint m_panStart;
    QPoint m_panStartScroll;
    void zoomAt(const QPoint& pos, double factor);
};

// ------------------------------------------------------------ main window
class ArtPlusMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit ArtPlusMainWindow(QWidget* parent = nullptr);
    ~ArtPlusMainWindow() override = default;
    bool openPdfPath(const QString& path);
    void viewPage(int index); // 0-based; triggers onPageChanged via the combo

private slots:
    void openPdf();
    void openArtPlus();
    void saveArtPlus();
    void exportPdf();
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void selectTool();
    void handTool();
    void zoomTool();
    void selectAll();
    void deselectAll();
    void deleteSelected();
    void onSelectionChanged(const std::vector<ObjectId>& ids);
    void updateModifiedIndicator();
    void onPageChanged(int index);

private:
    ArtPlusScene* m_scene = nullptr;
    ArtPlusView* m_view = nullptr;
    Document m_document;
    QString m_originalPdfPath;
    QDockWidget* m_layersDock = nullptr;
    QListWidget* m_layersList = nullptr;
    QDockWidget* m_objectsDock = nullptr;
    QTreeWidget* m_objectTree = nullptr;
    QStatusBar* m_statusBar = nullptr;
    QComboBox* m_pageCombo = nullptr;
    std::vector<PageId> m_pageIds;

    void setupUi();
    void setupMenusAndToolbars();
    void updatePageCombo();
    void rebuildScene();
    void updateDocumentInfo();
};

} // namespace ArtPlus
