#include <QtTest/QtTest>
#include <iostream>
#include "../../src/document/document_model.h"

using namespace ArtPlus;

class DocumentModelTests : public QObject {
    Q_OBJECT
private slots:
    void testDocumentCreation() {
        std::cout << "Running testDocumentCreation..." << std::endl;
        Document doc;
        QVERIFY(doc.pageOrder().empty());
        QVERIFY(doc.layerOrder().empty());
        QVERIFY(doc.objects().empty());
        QVERIFY(!doc.modified());
        QVERIFY(doc.filePath().isEmpty());
        QVERIFY(!doc.currentPage());
        std::cout << "testDocumentCreation passed." << std::endl;
    }

    void testPageId() {
        std::cout << "Running testPageId..." << std::endl;
        Page page1;
        std::cout << "  page1 created." << std::endl;
        PageId id1 = page1.id();
        std::cout << "  id1: " << id1 << std::endl;
        std::cout << "testPageId passed." << std::endl;
    }

    void testPageManagement() {
        std::cout << "Running testPageManagement..." << std::endl;
        Document doc;
        auto page1 = std::make_shared<Page>();
        auto page2 = std::make_shared<Page>();
        std::cout << "  Pages created." << std::endl;
        PageId id1 = page1->id();
        PageId id2 = page2->id();
        std::cout << "  IDs retrieved." << std::endl;

        doc.addPage(page1);
        doc.addPage(page2);
        std::cout << "  Pages added." << std::endl;

        QVERIFY(doc.page(id1) != nullptr);
        QVERIFY(doc.page(id2) != nullptr);
        QVERIFY(doc.currentPage()->id() == id1);

        doc.removePage(id1);
        std::cout << "testPageManagement passed." << std::endl;
    }

    void testObjectCreation() {
        std::cout << "Running testObjectCreation..." << std::endl;
        Document doc;
        auto path = std::make_shared<PathObject>();
        path->addMove(QPointF(0, 0));
        path->addLine(QPointF(100, 100));
        ObjectId pathId = path->id();
        doc.addObject(path);

        QCOMPARE(doc.objects().size(), (size_t)1);
        QVERIFY(doc.object(pathId) != nullptr);
        QCOMPARE(doc.object(pathId)->type(), ObjectType::Path);
        std::cout << "testObjectCreation passed." << std::endl;
    }

    void testCloning() {
        std::cout << "Running testCloning..." << std::endl;
        auto path = std::make_shared<PathObject>();
        path->setName("Original");
        path->addMove(QPointF(0, 0));

        auto cloned = path->clone();
        QVERIFY(cloned != nullptr);
        QVERIFY(cloned->id() != path->id());
        QCOMPARE(cloned->name(), QString("Original"));
        std::cout << "testCloning passed." << std::endl;
    }

    void testColors() {
        std::cout << "Running testColors..." << std::endl;
        Color cmyk = Color::cmyk(1.0f, 0.5f, 0.0f, 0.2f);
        QCOMPARE(cmyk.type, Color::Type::CMYK);
        QCOMPARE(cmyk.components.size(), 4);
        QVERIFY(cmyk.isValid());
        std::cout << "testColors passed." << std::endl;
    }
};

QTEST_APPLESS_MAIN(DocumentModelTests)
#include "test_main.moc"