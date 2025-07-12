#include <QApplication>
#include "client_app.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ClientApp window;
    window.show();
    return app.exec();
}


// make clean
// /opt/homebrew/opt/qt/bin/qmake client_gui.pro
// make
// ./client_gui.app/Contents/MacOS/client_gui
