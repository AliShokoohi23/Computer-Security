#include "mainwindow.h"
#include "keydialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KeyDialog k;
    if (!k.exec()) {
        return 1;
    }
    MainWindow w;
    w.show();
    return a.exec();
}
