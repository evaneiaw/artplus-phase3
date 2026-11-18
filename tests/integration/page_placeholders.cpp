#include "../../src/pdf/pdf_document.h"
#include "../../src/pdf/pdf_content.h"
#include <iostream>
#include <QCoreApplication>

using namespace ArtPlus;
using namespace ArtPlus::PDF;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    PdfDocument doc;
    doc.open(QString::fromLatin1("H:\\Desktop\\ArtPlus\\tests\\fixtures\\test_form_transform.pdf"));

    QByteArray contents = doc.pageContents(0);
    PdfValue res = doc.pageResources(0);
    auto r = PdfContentParser::parse(contents, doc, res, QTransform(), 841.8898);

    std::cout << "sequence=" << r.sequence.size() << " paths=" << r.paths.size() << " images=" << r.images.size() << "\n";
    for (const auto& ev : r.sequence) {
        if (ev.t == Event::T::Image) {
            const ParsedImage& pi = r.images[ev.index];
            const QTransform& t = pi.transform;
            std::cout << "Do name=" << pi.resourceName.toStdString()
                      << " t=[" << t.m11() << " " << t.m12() << " " << t.m21() << " " << t.m22()
                      << " | " << t.m31() << " " << t.m32() << "]\n";
        } else if (ev.t == Event::T::Path) {
            const ParsedPath& pp = r.paths[ev.index];
            std::cout << "Path elems=" << pp.elements.size() << " pts:";
            for (const auto& e : pp.elements) std::cout << "(" << e.p1.x() << "," << e.p1.y() << ") ";
            std::cout << "\n";
        }
    }
    return 0;
}
