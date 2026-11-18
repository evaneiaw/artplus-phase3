#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QDateTime>
#include <functional>
#include <memory>

namespace ArtPlus {

class Document;
class ActionListEngine;

class ScriptEngine : public QObject {
    Q_OBJECT
    
public:
    enum class Language {
        JavaScript = 0,
        Python = 1,
        Lua = 2
    };
    
    struct ScriptResult {
        bool success = false;
        QString error;
        QVariant result;
        qint64 executionTimeMs = 0;
    };
    
    struct ScriptInfo {
        QString name;
        QString path;
        QString description;
        QString author;
        QString version;
        QDateTime created;
        QDateTime modified;
        Language language;
        bool isBuiltIn = false;
    };
    
    struct ExecutionContext {
        Document* document = nullptr;
        QVariantMap globals;
        int maxExecutionTimeMs = 30000;
        bool allowFileIO = false;
        bool allowNetwork = false;
        std::function<void(const QString&)> printCallback;
        std::function<void(int, const QString&)> progressCallback;
    };
    
    explicit ScriptEngine(QObject* parent = nullptr);
    ~ScriptEngine();
    
    // Execution
    ScriptResult execute(const QString& code, ExecutionContext& context);
    ScriptResult executeFile(const QString& filePath, ExecutionContext& context);
    ScriptResult evaluate(const QString& expression, ExecutionContext& context);
    
    // Script management
    bool loadScript(const QString& filePath);
    bool unloadScript(const QString& name);
    bool reloadScript(const QString& name);
    QStringList loadedScripts() const;
    ScriptInfo scriptInfo(const QString& name) const;
    
    // API registration
    void registerGlobalObject(const QString& name, QObject* object);
    void registerGlobalFunction(const QString& name, std::function<QVariant(const QVariantList&)> func);
    void registerType(const QString& name, std::function<QObject*(const QVariantList&)> constructor);
    
    // ArtPlus API
    void registerDocumentAPI(Document* document);
    void registerActionListEngine(ActionListEngine* engine);
    void registerColorManager(class ColorManager* manager);
    void registerTrappingEngine(class TrappingEngine* engine);
    void registerImpositionEngine(class ImpositionEngine* engine);
    void registerMarksEngine(class MarksEngine* engine);
    void registerPackagingEngine(class PackagingEngine* engine);
    
    // Event handling
    void connectSignal(const QString& signal, const QString& scriptFunction);
    void disconnectSignal(const QString& signal);
    void emitSignal(const QString& signal, const QVariantList& args);
    
    // Timer/Async
    int setTimeout(const QString& functionName, int ms);
    int setInterval(const QString& functionName, int ms);
    void clearTimeout(int timerId);
    void clearInterval(int intervalId);
    
    // Console
    void setPrintCallback(std::function<void(const QString&)> callback);
    void log(const QString& message);
    void warn(const QString& message);
    void error(const QString& message);
    
    // Console API for scripts
    struct Console {
        static void log(const QString& message);
        static void warn(const QString& message);
        static void error(const QString& message);
        static void info(const QString& message);
        static void debug(const QString& message);
        static void time(const QString& label);
        static void timeEnd(const QString& label);
        static void trace();
        static void clear();
    };
    
    // File system (if allowed)
    struct FileSystem {
        static bool readFile(const QString& path, QString& content);
        static bool writeFile(const QString& path, const QString& content);
        static bool appendFile(const QString& path, const QString& content);
        static bool exists(const QString& path);
        static bool remove(const QString& path);
        static QStringList list(const QString& path);
        static bool mkdir(const QString& path);
        static bool rmdir(const QString& path);
        static qint64 size(const QString& path);
        static QDateTime modified(const QString& path);
    };
    
    // JSON
    struct JSON {
        static QVariant parse(const QString& text);
        static QString stringify(const QVariant& value, int indent = 0);
    };
    
    // Forward declarations for API classes
class DocumentAPI;
class PageAPI;
class LayerAPI;
class ObjectAPI;
class ColorAPI;
class GeometryAPI;

// Document API (exposed to scripts)
class DocumentAPI {
public:
    explicit DocumentAPI(Document* doc);
    
    QString name() const;
    QString filePath() const;
    bool isModified() const;
    int pageCount() const;
    int layerCount() const;
    int objectCount() const;
    
    QVariant page(int index);
    QVariant layer(const QString& name);
    QVariant object(const QString& id);
    
    bool save();
    bool saveAs(const QString& path);
    bool exportPDF(const QString& path);
    
    QVariantList selectedObjects();
    void selectAll();
    void deselectAll();
    void selectObject(const QString& id);
    void deselectObject(const QString& id);
    
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    
    // Signals as callbacks
    std::function<void(const QString&)> onObjectCreated;
    std::function<void(const QString&)> onObjectDeleted;
    std::function<void(const QString&)> onObjectModified;
    std::function<void()> onSelectionChanged;
    std::function<void()> onDocumentModified;
    
private:
    Document* m_document;
};

// Page API
class PageAPI {
public:
    explicit PageAPI(class Page* page);
    
    int index() const;
    QString name() const;
    QSizeF size() const;
    QRectF bounds() const;
    int layerCount() const;
    int objectCount() const;
    
    QVariant layer(const QString& name);
    QVariant object(const QString& id);
    
    bool addLayer(const QString& name);
    bool removeLayer(const QString& name);
    
    std::function<void(const QString&)> onObjectAdded;
    std::function<void(const QString&)> onObjectRemoved;
    
private:
    class Page* m_page;
};

// Layer API
class LayerAPI {
public:
    explicit LayerAPI(class Layer* layer);
    
    QString name() const;
    bool visible() const;
    void setVisible(bool visible);
    bool locked() const;
    void setLocked(bool locked);
    QColor color() const;
    void setColor(const QColor& color);
    float opacity() const;
    void setOpacity(float opacity);
    bool printable() const;
    void setPrintable(bool printable);
    int objectCount() const;
    QVariant object(const QString& id);
    QVariantList objects();
    
    bool addObject(const QString& objectId);
    bool removeObject(const QString& objectId);
    
    std::function<void(const QString&)> onObjectAdded;
    std::function<void(const QString&)> onObjectRemoved;
    std::function<void()> onChanged;
    
private:
    class Layer* m_layer;
};

// Object API
class ObjectAPI {
public:
    explicit ObjectAPI(class DocumentObject* obj);
    
    QString id() const;
    QString name() const;
    void setName(const QString& name);
    int type() const;
    QString typeName() const;
    bool visible() const;
    void setVisible(bool visible);
    bool locked() const;
    void setLocked(bool locked);
    QTransform transform() const;
    void setTransform(const QTransform& transform);
    QRectF bounds() const;
    
    void move(float dx, float dy);
    void scale(float sx, float sy);
    void rotate(float angle);
    void skew(float hx, float hy);
    void flipHorizontal();
    void flipVertical();
    
    QVariant fillColor() const;
    void setFillColor(const QVariant& color);
    QVariant strokeColor() const;
    void setStrokeColor(const QVariant& color);
    float strokeWidth() const;
    void setStrokeWidth(float width);
    
    bool clone();
    bool deleteObject();
    
    std::function<void()> onChanged;
    std::function<void()> onDeleted;
    
private:
    DocumentObject* m_object;
};

// Color API
class ColorAPI {
public:
    static QVariant createCMYK(float c, float m, float y, float k);
    static QVariant createRGB(float r, float g, float b);
    static QVariant createLab(float l, float a, float b);
    static QVariant createSpot(const QString& name, float tint = 1.0f);
    static QVariant createGray(float gray);
    
    static QVariant toCMYK(const QVariant& color);
    static QVariant toRGB(const QVariant& color);
    static QVariant toLab(const QVariant& color);
    static QVariant toGray(const QVariant& color);
    
    static float difference(const QVariant& c1, const QVariant& c2);
    static bool equal(const QVariant& c1, const QVariant& c2, float tolerance = 1.0f);
};

// Math/Geometry API
class GeometryAPI {
public:
    static QVariant point(float x, float y);
    static QVariant rect(float x, float y, float w, float h);
    static QVariant transform();
    
    static QVariant translate(const QVariant& transform, float dx, float dy);
    static QVariant scale(const QVariant& transform, float sx, float sy);
    static QVariant rotate(const QVariant& transform, float angle);
    static QVariant multiply(const QVariant& a, const QVariant& b);
    static QVariant invert(const QVariant& transform);
    
    static QVariant pointFromString(const QString& str);
    static QString pointToString(const QVariant& point);
};
    
private:
    struct Private;
    std::unique_ptr<Private> d;
};

namespace ScriptUtils {
    QString readFile(const QString& path);
    bool writeFile(const QString& path, const QString& content);
    QStringList findScripts(const QString& directory, const QStringList& extensions = {"js", "py", "lua"});
    QString generateScriptTemplate(const QString& type);
    QVariantMap scriptResultToMap(const QVariant& result);
    QString formatError(const QString& message, int line = -1, int column = -1);
}

} // namespace ArtPlus