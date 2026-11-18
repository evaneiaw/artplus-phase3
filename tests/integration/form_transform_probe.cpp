#include "../../src/pdf/pdf_document.h"
#include "../../src/pdf/pdf_importer.h"
#include "../../src/document/document_model.h"
#include <iostream>
#include <iomanip>
#include <QCoreApplication>

using namespace ArtPlus;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    PDF::PdfDocument doc;
    if (!doc.open(QString::fromLatin1("H:\\Desktop\\ArtPlus\\tests\\fixtures\\test_form_transform.pdf"))) {
        std::cout << "failed open\n";
        return 1;
    }
    Document model;
    auto res = PdfImporter::import(doc, model);
    std::cout << "pages=" << res.pagesImported << " objects=" << res.objectsImported << " warnings=" << res.warnings.size() << "\n";
    for (const auto& w : res.warnings) std::cout << "  warning: " << w.toStdString() << "\n";
    for (const auto& [id, obj] : model.objects()) {
        QString t = (obj->type() == ObjectType::Path ? "Path" : obj->type() == ObjectType::Text ? "Text" : obj->type() == ObjectType::Group ? "Group" : obj->type() == ObjectType::ClippingMask ? "Mask" : "Other");
        QTransform tr = obj->transform();
        std::cout << "id=" << id << " type=" << t.toStdString() << " parent=" << obj->parentId()
                  << " children=" << obj->children().size()
                  << " transform=[" << std::setprecision(4)
                  << tr.m11() << " " << tr.m12() << " " << tr.m21() << " " << tr.m22()
                  << " | " << tr.m31() << " " << tr.m32() << "]\n";
        if (obj->type() == ObjectType::Path) {
            auto p = std::dynamic_pointer_cast<PathObject>(obj);
            std::cout << "      elems=" << p->elements().size() << " closed=" << p->isClosed() << " pts:";
            for (const auto& e : p->elements()) {
                std::cout << "(" << e.p1.x() << "," << e.p1.y() << ") ";
            }
            std::cout << "\n";
            QRectF bb;
            for (const auto& e : p->elements()) {
                QPointF pts[3] = {e.p1, e.p2, e.p3};
                for (int k = 0; k < 3; ++k) {
                    QPointF mapped = obj->transform().map(pts[k]);
                    if (k == 0) bb = QRectF(mapped, mapped); else bb = bb.united(QRectF(mapped, mapped));
                }
            }
            std::cout << "      mapped bounds x:[" << bb.left() << ", " << bb.right()
                      << "] y:[" << bb.top() << ", " << bb.bottom() << "]\n";
        }
    }
    return 0;
}
