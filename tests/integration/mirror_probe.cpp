#include "../../src/pdf/pdf_content.h"
#include <iostream>
#include <QCoreApplication>

using namespace ArtPlus;
using namespace ArtPlus::PDF;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const char* stream =
        "q\n"
        "-1 0 0 1 500 600 cm\n"
        "/NestedForm Do\n"
        "Q\n"
        "q\n"
        "1.5 0 0 1.5 50 450 cm\n"
        "/NestedForm Do\n"
        "Q\n"
        "q\n"
        "-1 0 0 1 500 600 cm\n"
        "1 0 0 1 10 10 cm\n"
        "/NestedForm Do\n"
        "Q\n";
    auto r = PdfContentParser::parse(QByteArray(stream), {}, {}, QTransform(), 841.89);
    std::cout << "images=" << r.images.size() << "\n";
    for (const auto& img : r.images) {
        const QTransform& t = img.transform;
        std::cout << "Do name=" << img.resourceName.toStdString()
                  << " t=[" << t.m11() << " " << t.m12() << " " << t.m21() << " " << t.m22()
                  << " | " << t.m31() << " " << t.m32() << "]\n";
    }
    return 0;
}
