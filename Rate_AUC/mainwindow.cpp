#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    if (arg1 == 2) {
        ui->lineEdit_2->setEchoMode(QLineEdit::Password);
    } else if (arg1 == 0) {
        ui->lineEdit_2->setEchoMode(QLineEdit::Normal);
    }
}

