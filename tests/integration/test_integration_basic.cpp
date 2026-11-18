#include <QtTest/QtTest>
#include <iostream>
#include <filesystem>
#include "../../src/pdf/pdf_document.h"
#include "../../src/pdf/pdf_importer.h"
#include "../../src/pdf/pdf_exporter.h"
#include "../../src/document/document_model.h"
#include "../../src/document/document_serializer.h"

using namespace ArtPlus;

static QString fixtureDir() {
    const char* env = qgetenv("ARTPLUS_FIXTURES");
    if (env && *env) return QString::fromLatin1(env);
    return QString::fromLatin1("H:\\Desktop\\ArtPlus\\tests\\fixtures");
}

static QString testDir() {
    const char* env = qgetenv("ARTPLUS_TEST_OUTPUT");
    if (env && *env) return QString::fromLatin1(env);
    return QString::fromLatin1("H:\\Desktop\\ArtPlus\\build\\test_output");
}

class TestIntegrationBasic : public QObject {
    Q_OBJECT
private slots:
    void testOpenImportSaveRoundTrip_text() {
        // Teste: open PDF with text → import → save .artplus → reopen .artplus → verify text preserved
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_text.pdf"));

        Document model;
        auto res = PdfImporter::import(pdf, model);
        QVERIFY(res.ok);
        QCOMPARE(res.objectsImported, 3u);

        // Save as .artplus
        QString artplusPath = testDir() + "/test_text.artplus";
        QVERIFY(DocumentSerializer::save(model, artplusPath));

        // Reopen .artplus
        Document model2;
        QVERIFY(DocumentSerializer::load(artplusPath, model2));
        QVERIFY(model2.pageOrder().size() > 0);

        // Verify text objects preserved
        int textCount = 0;
        for (const auto& [id, obj] : model2.objects()) {
            if (obj->type() == ObjectType::Text) ++textCount;
        }
        QCOMPARE(textCount, 3);
    }

    void testOpenImportSaveRoundTrip_vector() {
        // Teste: open PDF with vectors → import → save .artplus → reopen .artplus → verify geometry preserved
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_vector.pdf"));

        Document model;
        auto res = PdfImporter::import(pdf, model);
        QVERIFY(res.ok);
        QCOMPARE(res.objectsImported, 6u);

        // Save as .artplus
        QString artplusPath = testDir() + "/test_vector.artplus";
        QVERIFY(DocumentSerializer::save(model, artplusPath));

        // Reopen .artplus
        Document model2;
        QVERIFY(DocumentSerializer::load(artplusPath, model2));
        QVERIFY(model2.pageOrder().size() > 0);

        // Verify path objects preserved
        int pathCount = 0;
        for (const auto& [id, obj] : model2.objects()) {
            if (obj->type() == ObjectType::Path) ++pathCount;
        }
        QCOMPARE(pathCount, 5);
    }

    void testOpenImportSaveRoundTrip_image() {
        // Teste: open PDF with image → import → save .artplus → reopen .artplus → verify image preserved
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_images.pdf"));

        Document model;
        auto res = PdfImporter::import(pdf, model);
        QVERIFY(res.ok);
        QCOMPARE(res.objectsImported, 2u);

        // Save as .artplus
        QString artplusPath = testDir() + "/test_image.artplus";
        QVERIFY(DocumentSerializer::save(model, artplusPath));

        // Reopen .artplus
        Document model2;
        QVERIFY(DocumentSerializer::load(artplusPath, model2));
        QVERIFY(model2.pageOrder().size() > 0);

        // Verify image object preserved
        int imageCount = 0;
        for (const auto& [id, obj] : model2.objects()) {
            if (obj->type() == ObjectType::Image) ++imageCount;
        }
        QCOMPARE(imageCount, 1);
    }

    void testOpenImportSaveRoundTrip_multipage() {
        // Teste: open multipage PDF → import first page → save → reopen → verify page count
        PDF::PdfDocument pdf;
        QVERIFY(pdf.open(fixtureDir() + "/test_multipage.pdf"));

        Document model;
        auto res = PdfImporter::import(pdf, model);
        QVERIFY(res.ok);
        // test_multipage.pdf has 3 pages; importer may import first page only or all
        QCOMPARE(model.pageOrder().size(), pdf.pageCount());

        // Save as .artplus
        QString artplusPath = testDir() + "/test_multipage.artplus";
        QVERIFY(DocumentSerializer::save(model, artplusPath));

        // Reopen .artplus
        Document model2;
        QVERIFY(DocumentSerializer::load(artplusPath, model2));
        QVERIFY(model2.pageOrder().size() > 0);

        // Verify pages preserved
        QCOMPARE(model2.pageOrder().size(), model.pageOrder().size());
    }
};

QTEST_APPLESS_MAIN(TestIntegrationBasic)
#include "test_integration_basic.moc"