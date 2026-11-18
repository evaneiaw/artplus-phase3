#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>
#include "../ui/main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption(QCommandLineOption("open", "Open PDF file on startup", "pdf"));
    parser.addOption(QCommandLineOption("page", "Switch to page N (1-based) after opening", "n"));
    parser.addOption(QCommandLineOption("quit-after", "Quit after N seconds (for smoke testing)", "sec"));
    parser.process(app);

    ArtPlus::ArtPlusMainWindow window;
    window.show();

    if (parser.isSet("open")) {
        QString path = parser.value("open");
        if (!window.openPdfPath(path)) {
            qCritical("openPdfPath failed for: %s", qPrintable(path));
            return 2;
        }
        if (parser.isSet("page")) {
            bool ok = false;
            int page = parser.value("page").toInt(&ok);
            if (ok && page >= 1) window.viewPage(page - 1);
        }
    }
    if (parser.isSet("quit-after")) {
        int sec = parser.value("quit-after").toInt();
        QTimer::singleShot(sec * 1000, &app, &QApplication::quit);
    }
    return app.exec();
}
