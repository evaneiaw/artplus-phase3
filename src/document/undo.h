#pragma once

#include "document_model.h"
#include <memory>
#include <vector>
#include <functional>

namespace ArtPlus {

// ============================================================ commands
// Undo/Redo is implemented as a command stack on Document. Each command
// captures enough state to reverse its own effect.

class Document;

class Command {
public:
    virtual ~Command() = default;
    virtual void undo(Document& doc) const = 0;
    virtual void redo(Document& doc) const = 0; // usually identical to execute
    virtual QString description() const = 0;
    virtual bool isExecutable() const { return true; }
};

// Move/translate/scale/rotate command for a set of object ids
struct TransformCommand : public Command {
    std::vector<ObjectId> ids;
    std::vector<QTransform> before;
    std::vector<QTransform> after;
    QString desc;

    TransformCommand() = default;
    TransformCommand(std::vector<ObjectId> i, std::vector<QTransform> b, std::vector<QTransform> a, const QString& d)
        : ids(std::move(i)), before(std::move(b)), after(std::move(a)), desc(d) {}

    QString description() const override { return desc; }
    void undo(Document& doc) const override;
    void redo(Document& doc) const override;
};

// Delete command: snapshot cloned objects with their layer membership and index.
struct DeleteObjectsCommand : public Command {
    std::vector<std::pair<ObjectId, std::shared_ptr<DocumentObject>>> snapshots;
    std::vector<std::pair<ObjectId, int>> layerIndexPerObject; // index in the owning layer
    std::vector<std::pair<ObjectId, LayerId>> layerPerObject;  // first owning layer

    void undo(Document& doc) const override;
    void redo(Document& doc) const override;
    QString description() const override { return QString("Delete %1 object(s)").arg(snapshots.size()); }

    static std::unique_ptr<DeleteObjectsCommand> capture(Document& doc, const std::vector<ObjectId>& ids);
};

// Text edit command (for text objects)
struct EditTextCommand : public Command {
    ObjectId id = 0;
    QString before, after;

    EditTextCommand() = default;
    EditTextCommand(ObjectId i, const QString& b, const QString& a) : id(i), before(b), after(a) {}

    QString description() const override { return "Edit text"; }
    void undo(Document& doc) const override;
    void redo(Document& doc) const override;
};

// Visibility command (toggle)
struct SetVisibilityCommand : public Command {
    ObjectId id = 0;
    bool before = true, after = true;

    SetVisibilityCommand() = default;
    SetVisibilityCommand(ObjectId i, bool b, bool a) : id(i), before(b), after(a) {}

    QString description() const override { return "Toggle visibility"; }
    void undo(Document& doc) const override;
    void redo(Document& doc) const override;
};

class UndoStack {
public:
    UndoStack() = default;

    void push(std::unique_ptr<Command> cmd);
    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    QString undoDescription() const { return m_undo.empty() ? QString() : m_undo.back()->description(); }
    QString redoDescription() const { return m_redo.empty() ? QString() : m_redo.back()->description(); }

    void undo(Document& doc);
    void redo(Document& doc);
    void clear() { m_undo.clear(); m_redo.clear(); }

private:
    std::vector<std::unique_ptr<Command>> m_undo;
    std::vector<std::unique_ptr<Command>> m_redo;
};

} // namespace ArtPlus
