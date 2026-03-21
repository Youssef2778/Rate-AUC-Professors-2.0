#include "mainwindow.h"

#include <QApplication>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow* w = new MainWindow;
    QLabel* login_image = new QLabel;

    // (make the window fill the entire monitor)
    w.showMaximized();

    return a.exec();
}
