#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>

class ArtPlusMainWindow : public QMainWindow {
public:
    ArtPlusMainWindow() {
        setWindowTitle("ArtPlus - Professional Prepress Editor");
        resize(1200, 800);

        setupMenus();
        setupToolBars();
        
        statusBar()->showMessage("Ready");
    }

private:
    void setupMenus() {
        QMenu *fileMenu = menuBar()->addMenu("&File");
        fileMenu->addAction("&Open");
        fileMenu->addAction("&Save");
        fileMenu->addSeparator();
        fileMenu->addAction("&Exit", qApp, &QCoreApplication::quit);

        QMenu *editMenu = menuBar()->addMenu("&Edit");
        editMenu->addAction("&Undo");
        editMenu->addAction("&Redo");

        QMenu *viewMenu = menuBar()->addMenu("&View");
        QMenu *prepressMenu = menuBar()->addMenu("&Prepress");
    }

    void setupToolBars() {
        QToolBar *mainToolBar = addToolBar("Main");
        mainToolBar->addAction("Select");
        mainToolBar->addAction("Zoom");
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ArtPlusMainWindow window;
    window.show();
    return app.exec();
}
