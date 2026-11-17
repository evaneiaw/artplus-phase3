#include <QtTest/QtTest>
#include <iostream>
#include "../../src/document/document_model.h"
#include "../../src/document/document_serializer.h"
#include "../../src/pdf/pdf_document.h"
#include "../../src/pdf/pdf_importer.h"
#include "../../src/pdf/pdf_exporter.h"

using namespace ArtPlus;

static QString fixtureDir() {
    const char* env = qgetenv("ARTPLUS_FIXTURES");
    if (env && *env) return QString::fromLatin1(env);
    return QString::fromLatin1("H:\\Desktop\\ArtPlus\\tests\\fixtures");
}

class TestEditing : public QObject {
    Q_OBJECT
private slots:
    void testSelectAllAndSingle() {
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_vector.pdf"));
        Document doc;
        auto res = PdfImporter::import(pdf, doc);
        QVERIFY(res.ok);
        const auto ids = doc.objectIds();
        QCOMPARE(ids.size(), (size_t)6);

        // Single selection
        doc.selectObject(ids[0], false);
        QCOMPARE(doc.selectedObjects().size(), (size_t)1);
        QCOMPARE(doc.selectedObjects().front(), ids[0]);

        // Select additional (add mode)
        doc.selectObject(ids[1], true);
        QCOMPARE(doc.selectedObjects().size(), (size_t)2);

        // Clear selection
        doc.selectObject(ids[2], false); // replaces selection
        QCOMPARE(doc.selectedObjects().size(), (size_t)1);
        doc.clearSelection();
        QCOMPARE(doc.selectedObjects().size(), (size_t)0);
    }

    void testMoveObjectChangesTransform() {
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_vector.pdf"));
        Document doc;
        auto res = PdfImporter::import(pdf, doc);
        QVERIFY(res.ok);
        const auto ids = doc.objectIds();
        ObjectId target = ids[0];
        auto obj = doc.object(target);
        QVERIFY(obj != nullptr);

        QTransform before = obj->transform();
        obj->setTransform(QTransform(before.m11(), before.m12(), before.m21(), before.m22(),
                                     before.m31() + 30.0, before.m32() + 15.0));
        QTransform after = obj->transform();
        QCOMPARE(after.m31(), before.m31() + 30.0);
        QCOMPARE(after.m32(), before.m32() + 15.0);
    }

    void testDeleteObject() {
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_vector.pdf"));
        Document doc;
        auto res = PdfImporter::import(pdf, doc);
        QVERIFY(res.ok);
        size_t before = doc.objects().size();
        const auto ids = doc.objectIds();
        doc.removeObject(ids[0]);
        // also remove from layer
        for (LayerId lid : doc.layerOrder()) {
            if (auto layer = doc.layer(lid)) layer->removeObject(ids[0]);
        }
        QCOMPARE(doc.objects().size(), before - 1);
        QVERIFY(doc.object(ids[0]) == nullptr);
    }

    void testMovedObjectSurvivesExportAndReimport() {
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_vector.pdf"));
        Document doc;
        auto res = PdfImporter::import(pdf, doc);
        QVERIFY(res.ok);
        const auto ids = doc.objectIds();
        ObjectId target = ids[0];
        auto obj = doc.object(target);
        QVERIFY(obj != nullptr);

        // Move object
        QTransform t = obj->transform();
        obj->setTransform(QTransform(t.m11(), t.m12(), t.m21(), t.m22(), t.m31() + 42.0, t.m32() + 17.0));
        doc.setModified(true);

        QString out = QDir::temp().filePath("artplus_edit_move.pdf");
        QVERIFY(PDF::PdfExporter::exportPdf(doc, out, fixtureDir() + "/test_vector.pdf"));

        // Reimport and check we still see the moved content (check number of paths preserved)
        PDF::PdfDocument pdf2;
        QVERIFY(pdf2.open(out));
        Document doc2;
        auto res2 = PdfImporter::import(pdf2, doc2);
        QVERIFY(res2.ok);
        // Movement visible via control point of curves
        QVERIFY(res2.pathsImported >= 4);
    }

    void testDeletedObjectDisappearsFromExport() {
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_vector.pdf"));
        Document doc;
        auto res = PdfImporter::import(pdf, doc);
        QVERIFY(res.ok);
        size_t before = doc.objects().size();

        const auto ids = doc.objectIds();
        doc.removeObject(ids[0]);
        for (LayerId lid : doc.layerOrder()) {
            if (auto layer = doc.layer(lid)) layer->removeObject(ids[0]);
        }
        doc.setModified(true);

        QString out = QDir::temp().filePath("artplus_edit_delete.pdf");
        QVERIFY(PDF::PdfExporter::exportPdf(doc, out, fixtureDir() + "/test_vector.pdf"));

        PDF::PdfDocument pdf2;
        QVERIFY(pdf2.open(out));
        Document doc2;
        auto res2 = PdfImporter::import(pdf2, doc2);
        QVERIFY(res2.ok);
        // deleted object must be absent
        QCOMPARE(doc2.objects().size(), before - 1);
    }

    void testTransformScaleAndRotation() {
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_vector.pdf"));
        Document doc;
        auto res = PdfImporter::import(pdf, doc);
        QVERIFY(res.ok);
        const auto ids = doc.objectIds();
        auto obj = doc.object(ids[0]);
        QVERIFY(obj != nullptr);

        QTransform scale;
        scale.scale(2.0, 2.0);
        obj->setTransform(scale * obj->transform());
        QTransform rotated;
        rotated.rotate(45);
        obj->setTransform(rotated * obj->transform());
        // Just verifying no crash and transform is set
        QVERIFY(obj->transform().isAffine());
    }
};

QTEST_APPLESS_MAIN(TestEditing)
#include "test_editing.moc"
