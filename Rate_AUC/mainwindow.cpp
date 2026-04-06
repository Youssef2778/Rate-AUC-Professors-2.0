#include "mainwindow.h"
#include "./ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
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

void MainWindow::on_register_label_4_linkActivated(const QString &link)
{
    ui->stackedWidget->setCurrentIndex(1);

    // Hiding the error messages.
    ui->empty_email_error->hide();
    ui->empty_username_error->hide();
    ui->auc_email_error->hide();
    ui->empty_pass_error->hide();
    ui->empty_confPass_error->hide();
    ui->unequal_pass_error->hide();


    // Assume you have a widget inside the stacked page
    QWidget* page = ui->stackedWidget->widget(1); // second page
    QVBoxLayout* vLayout = new QVBoxLayout(page);

    // Create a horizontal layout for centering
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addStretch();            // left spacer
    hLayout->addWidget(ui->widget_2); // your target widget
    hLayout->addStretch();            // right spacer

    vLayout->addStretch();   // top spacer
    vLayout->addLayout(hLayout);
    vLayout->addStretch();   // bottom spacer

    page->setLayout(vLayout);
}


void MainWindow::on_checkBox_6_stateChanged(int arg1)
{
    if (arg1 == 2) {
        ui->password_register_lineEdit->setEchoMode(QLineEdit::Password);
        ui->confPassword_register_lineEdit->setEchoMode(QLineEdit::Password);
    } else if (arg1 == 0) {
        ui->password_register_lineEdit->setEchoMode(QLineEdit::Normal);
        ui->confPassword_register_lineEdit->setEchoMode(QLineEdit::Normal);
    }
}


void MainWindow::on_register_label_6_linkActivated(const QString &link)
{
    ui->stackedWidget->setCurrentIndex(0);

}

    // ui->empty_email_error->hide();
    // ui->empty_username_error->hide();
    // ui->auc_email_error->hide();
    // ui->empty_pass_error->hide();
    // ui->empty_confPass_error->hide();

void MainWindow::on_pushButton_6_clicked()
{
    // Client-side validation (presence check + format check of email)
    if (ui->username_register_lineEdit->text() == "") ui->empty_username_error->show();
    else ui->empty_username_error->hide();

    if (ui->password_register_lineEdit->text() == "") ui->empty_pass_error->show();
    else ui->empty_pass_error->hide();

    if (ui->confPassword_register_lineEdit->text() == "") ui->empty_confPass_error->show();
    else ui->empty_confPass_error->hide();

    if (ui->email_register_lineEdit->text() == "") ui->empty_email_error->show();
    else {
        ui->empty_email_error->hide();
        std::string email = ui->email_register_lineEdit->text().toStdString();
        int i = email.find('@');
        if (!i) ui->empty_email_error->show();
        else {
            if (email.substr(i + 1, email.size() - i - 1) != "aucegypt.edu") ui->auc_email_error->show();
            else ui->auc_email_error->hide();
        }
    }

    if (ui->password_register_lineEdit->text() != ui->confPassword_register_lineEdit->text()) ui->unequal_pass_error->show();
    else ui->unequal_pass_error->hide();


}

