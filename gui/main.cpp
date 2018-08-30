#include <QApplication>
#include <QFont>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QFont f = app.font();
    //f.setFamily("Monaco");
    f.setPointSize(8);
    app.setFont(f);

    MainWindow window;
    // normal display
    //window.show();
    // nice maximal size display by default
    window.showMaximized();
    // hard full screen
    //window.showFullScreen();

    return app.exec();
}
