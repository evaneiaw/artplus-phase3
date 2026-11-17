#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>
#include "../ui/main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption(QCommandLineOption("open", "Open PDF file on startup", "pdf"));
    parser.addOption(QCommandLineOption("quit-after", "Quit after N seconds (for smoke testing)", "sec"));
    parser.process(app);

    ArtPlus::ArtPlusMainWindow window;
    window.show();

    if (parser.isSet("open")) {
        QString path = parser.value("open");
        window.openPdfPath(path);
    }
    if (parser.isSet("quit-after")) {
        int sec = parser.value("quit-after").toInt();
        QTimer::singleShot(sec * 1000, &app, &QApplication::quit);
    }
    return app.exec();
}
