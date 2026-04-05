#include "mainwindow.h"
#include "./ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // The following lines of code are produced by ChatGPT, their purpose is to constantly center the form no matter the size
    // of the main window


    // Assume you have a widget inside the stacked page
    QWidget* page = ui->stackedWidget->widget(0); // first page
    QVBoxLayout* vLayout = new QVBoxLayout(page);

    // Create a horizontal layout for centering
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addStretch();            // left spacer
    hLayout->addWidget(ui->widget); // your target widget
    hLayout->addStretch();            // right spacer

    vLayout->addStretch();   // top spacer
    vLayout->addLayout(hLayout);
    vLayout->addStretch();   // bottom spacer

    page->setLayout(vLayout);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_checkBox_4_stateChanged(int arg1)
{
    if (arg1 == 2) {
        ui->lineEdit_8->setEchoMode(QLineEdit::Password);
    } else if (arg1 == 0) {
        ui->lineEdit_8->setEchoMode(QLineEdit::Normal);
    }
}


// void MainWindow::on_register_label_linkActivated(const QString &link)
// {
//     this->hide();
// }





void MainWindow::on_register_label_4_linkActivated(const QString &link)
{
    ui->stackedWidget->setCurrentIndex(1);
}

