#pragma once

#include "document_model.h"
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <memory>
#include <vector>

namespace ArtPlus {

// Serialize/deserialize the ArtPlus document model to a JSON file (.artplus).
// The format is stable and versioned; unknown keys are preserved on round-trip.
class DocumentSerializer {
public:
    static QJsonObject toJson(const Document& doc);
    static bool fromJson(const QJsonObject& json, Document& out);

    static bool save(const Document& doc, const QString& filePath);
    static bool load(const QString& filePath, Document& out);

    // Integrity check between two documents (same structure/counts).
    struct IntegrityReport {
        bool ok = true;
        QStringList differences;
    };
    static IntegrityReport compare(const Document& a, const Document& b);
};

} // namespace ArtPlus
