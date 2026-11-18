#pragma once

#include "../document/document_model.h"
#include "../document/undo.h"
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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputDialog>
#include <QFileDialog>
#include <QLineEdit>
#include <memory>
#include <vector>
#include <unordered_map>

namespace ArtPlus {

// ---------------------------------------------------------------- scene
class ArtPlusScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit ArtPlusScene(QObject* parent = nullptr) : QGraphicsScene(parent) {}
    void setDocument(Document* doc);
    Document* document() const { return m_doc; }
    void setUndoStack(UndoStack* stack) { m_undoStack = stack; }
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
    UndoStack* m_undoStack = nullptr;
    bool m_dragging = false;
    QPointF m_dragStart;
    bool m_rubberBand = false;
    QRectF m_rubberBandRect;
    bool m_additive = false;
    std::unordered_map<ObjectId, QTransform> m_dragStartTransforms;

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

// Action structures for Module 17/18
struct Action {
    enum class Type {
        StepRepeat,
        Imposition,
        Marks,
        CutCreaseLines,
        Unknown
    } type;

    // Step & Repeat parameters
    int copiesX = 1;
    int copiesY = 1;
    qreal spacingX = 0.0;
    qreal spacingY = 0.0;
    qreal angle = 0.0;

    // Imposition parameters
    int impositionType = 0; // 0=None, 1=Half, 2=Quarter

    // Marks parameters
    int marksType = 0; // 0=None, 1=Crop, 2=Registration, 3=Crease

    // Cut/Crease Lines parameters
    int cutType = 0; // 0=None, 1=Cut, 2=Crease, 3=Art/Knife Separation

    // Data for serialization
    QString description;

    QJsonDocument toJson() const;
    static Action fromJson(const QJsonDocument& doc);
};

struct ActionList {
    QString name;
    QDateTime timestamp;
    QVector<Action> actions;

    QJsonDocument toJson() const;
    static ActionList fromJson(const QJsonDocument& doc);
};

class ArtPlusMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit ArtPlusMainWindow(QWidget* parent = nullptr);
    ~ArtPlusMainWindow() override = default;
    bool openPdfPath(const QString& path);
    void viewPage(int index); // 0-based; triggers onPageChanged via the combo

private slots:
    // File operations
    void openPdf();
    void openArtPlus();
    void saveArtPlus();
    void exportPdf();

    // View operations
    void zoomIn();
    void zoomOut();
    void zoomFit();

    // Tool selection
    void selectTool();
    void handTool();
    void zoomTool();

    // Selection operations
    void selectAll();
    void deselectAll();
    void deleteSelected();

    // Undo/Redo
    void undoAction();
    void redoAction();

    // Production operations (Module 14-16)
    void stepRepeat();
    void imposition();
    void marks();
    void cutLines();

    // Action Lists (Module 17)
    void actionLists();
    void executeAction(const Action& action);
    void batchProcessing();

    // Scripts & Extensibility (Module 18)
    void presets();
    void scripting();

    // Reports, Performance, Recovery (Module 19/20)
    void reports();
    void performance();
    void recovery();

    // Document state
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
    UndoStack m_history;
    QComboBox* m_pageCombo = nullptr;
    std::vector<PageId> m_pageIds;

    // Action Lists recording state
    QVector<Action> m_recordedActions;

    void setupUi();
    void setupMenusAndToolbars();
    void updatePageCombo();
    void rebuildScene();
    void updateDocumentInfo();
};

} // namespace ArtPlus
