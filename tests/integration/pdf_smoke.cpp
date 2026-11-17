#include "../../src/pdf/pdf_document.h"
#include "../../src/pdf/pdf_importer.h"
#include "../../src/document/document_model.h"
#include <iostream>
#include <QCoreApplication>

using namespace ArtPlus;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const char* files[] = {
        "H:\\Desktop\\ArtPlus\\tests\\fixtures\\test_text.pdf",
        "H:\\Desktop\\ArtPlus\\tests\\fixtures\\test_vector.pdf",
        "H:\\Desktop\\ArtPlus\\tests\\fixtures\\test_images.pdf",
        "H:\\Desktop\\ArtPlus\\tests\\fixtures\\test_multipage.pdf",
        "H:\\Desktop\\ArtPlus\\tests\\fixtures\\test_cmyk_inks.pdf",
    };

    for (const char* f : files) {
        std::cout << "==============================================\n";
        std::cout << "File: " << f << "\n";
        PDF::PdfDocument doc;
        if (!doc.open(QString::fromLatin1(f))) {
            std::cout << "FAILED to open\n";
            continue;
        }
        std::cout << "Pages in PDF: " << doc.pageCount() << "\n";
        Document document;
        PdfImportResult res = PdfImporter::import(doc, document);
        std::cout << "Import: pages=" << res.pagesImported
                  << " objects=" << res.objectsImported
                  << " paths=" << res.pathsImported
                  << " texts=" << res.textsImported
                  << " images=" << res.imagesImported
                  << " warnings=" << res.warnings.size() << "\n";
        for (const auto& w : res.warnings) {
            std::cout << "   warning: " << w.toStdString() << "\n";
        }
        std::cout << "Document model: pages=" << document.pageOrder().size()
                  << " layers=" << document.layerOrder().size()
                  << " objects=" << document.objects().size() << "\n";
        for (const auto& [id, obj] : document.objects()) {
            QString type;
            switch (obj->type()) {
                case ObjectType::Path: type = "Path"; break;
                case ObjectType::Text: type = "Text"; break;
                case ObjectType::Image: type = "Image"; break;
                case ObjectType::Group: type = "Group"; break;
                default: type = "Other";
            }
            QString info;
            if (obj->type() == ObjectType::Image) {
                auto imgObj = std::dynamic_pointer_cast<ImageObject>(obj);
                info = QString("%1x%2").arg(imgObj->image().width()).arg(imgObj->image().height());
            } else if (obj->type() == ObjectType::Text) {
                auto t = std::dynamic_pointer_cast<TextObject>(obj);
                info = t->text().left(40);
            } else if (obj->type() == ObjectType::Path) {
                auto p = std::dynamic_pointer_cast<PathObject>(obj);
                info = QString("elems=%1").arg(p->elements().size());
            }
            std::cout << "   object id=" << id << " type=" << type.toStdString()
                      << " name=" << obj->name().toStdString()
                      << " info=" << info.toStdString() << "\n";
        }
    }
    return 0;
}
