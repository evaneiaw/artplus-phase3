#include <QtTest/QtTest>
#include <iostream>
#include "../../src/pdf/pdf_document.h"
#include "../../src/pdf/pdf_importer.h"
#include "../../src/document/document_model.h"

using namespace ArtPlus;

static QString fixtureDir() {
    const char* env = qgetenv("ARTPLUS_FIXTURES");
    if (env && *env) return QString::fromLatin1(env);
    return QString::fromLatin1("H:\\Desktop\\ArtPlus\\tests\\fixtures");
}

class TestFormXObjects : public QObject {
    Q_OBJECT
private slots:
    void testFormXObjectsBecomeGroups() {
        PDF::PdfDocument doc;
        QVERIFY(doc.open(fixtureDir() + "/test_formxobject.pdf"));
        Document model;
        auto res = PdfImporter::import(doc, model);
        QVERIFY(res.ok);
        QVERIFY(res.objectsImported == 7);
        // Should contain group(s)
        int groups = 0, texts = 0, paths = 0;
        for (const auto& [id, obj] : model.objects()) {
            if (obj->type() == ObjectType::Group) ++groups;
            else if (obj->type() == ObjectType::Text) ++texts;
            else if (obj->type() == ObjectType::Path) ++paths;
        }
        QCOMPARE(groups, 2);
        QCOMPARE(texts, 3);
        QCOMPARE(paths, 2);
    }

    void testUnsupportedElementsProduceWarnings() {
        PDF::PdfDocument doc;
        QVERIFY(doc.open(fixtureDir() + "/test_unsupported.pdf"));
        Document model;
        auto res = PdfImporter::import(doc, model);
        QVERIFY(res.ok);
        // at least the annotation warning is expected
        bool hasAnnotationWarning = false;
        for (const auto& w : res.warnings) {
            if (w.contains("annotation", Qt::CaseInsensitive)) hasAnnotationWarning = true;
        }
        QVERIFY2(hasAnnotationWarning, "Expected an annotation warning for the fixture with /Annots");
    }
};

QTEST_APPLESS_MAIN(TestFormXObjects)
#include "form_xobject_test.moc"
