#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.show();

    return a.exec();
}
