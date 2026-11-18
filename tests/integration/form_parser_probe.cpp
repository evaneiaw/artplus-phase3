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

    // Form 4 dictionary is object 4 0 obj; find it
    const PdfObject* form = doc.findObject(4, 0);
    if (!form) { std::cout << "form not found\n"; return 1; }
    std::cout << "form isStream=" << form->isStream << " streamBytes=" << form->stream.size() << "\n";
    std::cout << "form dict: " << form->value.toString().toStdString() << "\n";

    QByteArray contents((const char*)form->stream.data(), (int)form->stream.size());
    std::cout << "content stream: [" << contents.toStdString() << "]\n";

    PdfValue res = form->value.dictGet("Resources");
    if (res.isRef()) res = doc.resolve(res);
    std::cout << "resources isDict=" << res.isDictionary() << "\n";

    // identity ctm, pageHeight=-1 (no flip)
    auto r = PdfContentParser::parse(contents, doc, res, QTransform(), -1.0);
    std::cout << "paths=" << r.paths.size() << " texts=" << r.texts.size() << " sequence=" << r.sequence.size() << "\n";
    for (const auto& p : r.paths) {
        std::cout << "  path fill=" << p.hasFill << " elems=" << p.elements.size() << "\n";
        for (const auto& e : p.elements) {
            std::cout << "    t=" << (int)e.t << " p1=(" << e.p1.x() << "," << e.p1.y() << ")\n";
        }
    }
    for (const auto& t : r.texts) {
        std::cout << "  text [" << t.text.toStdString() << "] transform=[" << t.transform.m11() << " " << t.transform.m12() << " " << t.transform.m13()
                  << " / " << t.transform.m21() << " " << t.transform.m22() << " " << t.transform.m23()
                  << " / " << t.transform.m31() << " " << t.transform.m32() << "]\n";
    }

    // Now with formCompose: formMatrix identity, placeholder=flip+translate(50,650)
    QTransform ph = QTransform(1, 0, 0, -1, 0, 841.89) * QTransform(1, 0, 0, 1, 50, 650);
    // note: placeholder at Do time = ctm = F∘cm
    // Qt applies cm first, then F, using cm * F order? verify order: we want p_user -> flip(cm(p))
    auto r2 = PdfContentParser::parse(contents, doc, res, ph, -1.0);
    std::cout << "== composed ==\n";
    std::cout << "paths=" << r2.paths.size() << "\n";
    for (const auto& p : r2.paths) {
        std::cout << "  path fill=" << p.hasFill << " elems=" << p.elements.size() << "\n";
        for (const auto& e : p.elements) {
            std::cout << "    t=" << (int)e.t << " p1=(" << e.p1.x() << "," << e.p1.y() << ")\n";
        }
    }
    return 0;
}
