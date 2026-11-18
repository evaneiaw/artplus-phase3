#include "../../src/pdf/pdf_document.h"
#include "../../src/pdf/pdf_importer.h"
#include "../../src/document/document_model.h"
#include <iostream>
#include <QCoreApplication>

using namespace ArtPlus;
using namespace ArtPlus::PDF;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    PDF::PdfDocument doc;
    if (!doc.open(QString::fromLatin1("H:\\Desktop\\ArtPlus\\tests\\fixtures\\test_smask.pdf"))) {
        std::cout << "FAILED open\n";
        return 1;
    }
    std::cout << "pages=" << doc.pageCount() << "\n";
    std::cout << "title=" << doc.info().title.toStdString() << "\n";

    // dump ExtGState resources and SMask ref
    PdfValue res = doc.pageResources(0);
    PdfValue gs = res.dictGet("ExtGState");
    std::cout << "ExtGState isDictionary=" << gs.isDictionary() << "\n";
    PdfValue gs1 = gs.dictGet("GS1");
    if (gs1.isRef()) gs1 = doc.resolve(gs1);
    std::cout << "GS1: " << gs1.toString().toStdString() << "\n";
    PdfValue smask = gs1.dictGet("SMask");
    std::cout << "SMask type: isDict=" << smask.isDictionary() << " isRef=" << smask.isRef() << "\n";
    PdfValue smaskDict = smask.isRef() ? doc.resolve(smask) : smask;
    std::cout << "SMask resolved: " << smaskDict.toString().toStdString() << "\n";
    PdfValue maskRef = smaskDict.dictGet("G");
    std::cout << "G isRef=" << maskRef.isRef() << " obj=" << maskRef.refObj() << " gen=" << maskRef.refGen() << "\n";
    if (maskRef.isRef()) {
        const PdfObject* xo = doc.findObject(maskRef.refObj(), maskRef.refGen());
        if (xo && xo->isStream) {
            std::cout << "mask xobject stream bytes=" << xo->stream.size() << "\n";
            std::cout << "Width=" << xo->value.getInt("Width", -1) << " Height=" << xo->value.getInt("Height", -1)
                      << " bpc=" << xo->value.getInt("BitsPerComponent", -1)
                      << " filter=" << xo->value.dictGet("Filter").toString().toStdString() << "\n";
        }
    }

    // Import into the model
    Document model;
    auto imp = PdfImporter::import(doc, model);
    std::cout << "import ok=" << imp.ok << " pages=" << imp.pagesImported << " objects=" << imp.objectsImported << " paths=" << imp.pathsImported << " texts=" << imp.textsImported << " images=" << imp.imagesImported << "\n";
    for (const auto& w : imp.warnings) std::cout << "  warning: " << w.toStdString() << "\n";
    for (const auto& [id, obj] : model.objects()) {
        QString t;
        switch (obj->type()) {
            case ObjectType::Path: t = "Path"; break;
            case ObjectType::Text: t = "Text"; break;
            case ObjectType::Image: t = "Image"; break;
            case ObjectType::Group: t = "Group"; break;
            case ObjectType::ClippingMask: t = "Mask"; break;
            case ObjectType::OpacityMask: t = "OpacityMask"; break;
        }
        std::cout << "  id=" << id << " type=" << t.toStdString() << " name=" << obj->name().toStdString() << "\n";
    }
    return 0;
}
