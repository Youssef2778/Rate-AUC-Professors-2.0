#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <iostream>
#include <fstream>
#include "bcrypt/BCrypt.hpp"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(1);
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
    // 1. Initialize the table structure
    ui->tableWidget->setColumnCount(5);
    ui->tableWidget->setRowCount(6);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setShowGrid(false);

    // 2. Loop to create the 6 professor "cards"
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        // Text Data
        QTableWidgetItem *rank = new QTableWidgetItem(QString::number(i + 1));
        QTableWidgetItem *name = new QTableWidgetItem("Professor " + QString::number(i + 1));
        QTableWidgetItem *score = new QTableWidgetItem("10 points");

        // 2. Now that they have names, we can center them
        rank->setTextAlignment(Qt::AlignCenter);
        name->setTextAlignment(Qt::AlignCenter);
        score->setTextAlignment(Qt::AlignCenter);

        // 3. Finally, put the finished items into the table
        ui->tableWidget->setItem(i, 0, rank);
        ui->tableWidget->setItem(i, 1, name);
        ui->tableWidget->setItem(i, 2, score);

        // Change the buttons to text or standard symbols
        QPushButton *up = new QPushButton("+ Upvote");
        QPushButton *down = new QPushButton("- Downvote");

        // Button Styling
        QString btnStyle = "QPushButton { background-color: #0b2239; color: white; border-radius: 5px; border: 1px solid #ffd700; font-family: 'Segoe UI Emoji'; }";
        up->setStyleSheet(btnStyle);
        down->setStyleSheet(btnStyle);

        // Put buttons in the correct columns
        ui->tableWidget->setCellWidget(i, 3, up);
        ui->tableWidget->setCellWidget(i, 4, down);

        // Match the row height to the design
        ui->tableWidget->setRowHeight(i, 60);
    }
} // the desin of the leaderboard was coassisted by AI in order to get the right color pallets and design down

MainWindow::~MainWindow()
{
    delete ui;
}

// "Hide Password" Mechanism of the Login page
void MainWindow::on_checkBox_4_stateChanged(int arg1)
{
    if (arg1 == 2) {
        ui->lineEdit_8->setEchoMode(QLineEdit::Password);
    } else if (arg1 == 0) {
        ui->lineEdit_8->setEchoMode(QLineEdit::Normal);
    }
}

// Function executed when the user moves to the register page.
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

// "Hide Password" Mechanism of the Register Page
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

// Function called when the "Register" Button in the Register page is clicked.
// This involves the networking logic.
void MainWindow::on_pushButton_6_clicked()
{
    // Client-side validation (presence check + format check of email)

    // Presence Checks
    if (ui->username_register_lineEdit->text() == "") {
        ui->empty_username_error->show();
        return;
    }
    else ui->empty_username_error->hide();

    if (ui->password_register_lineEdit->text() == "") {
        ui->empty_pass_error->show();
        return;
    }
    else ui->empty_pass_error->hide();

    if (ui->confPassword_register_lineEdit->text() == "") {
        ui->empty_confPass_error->show();
        return;
    }
    else ui->empty_confPass_error->hide();

    
    if (ui->email_register_lineEdit->text() == "") 
    {
        ui->empty_email_error->show();
        return;
    }
    else {
        ui->empty_email_error->hide();

        // Checking the format of the email..
        std::string email = ui->email_register_lineEdit->text().toStdString();
        int i = email.find('@');
        if (!i) ui->empty_email_error->show();
        else {
            if (email.substr(i + 1, email.size() - i - 1) != "aucegypt.edu") {
                ui->auc_email_error->show();
                return;
            }
            else ui->auc_email_error->hide();
        }
    }

    // Checking if the two passwords input are the same..
    if (ui->password_register_lineEdit->text() != ui->confPassword_register_lineEdit->text()) {
        ui->unequal_pass_error->show();
        return;
    }
    else ui->unequal_pass_error->hide();


    // Now, let's see what the server thinks about the data!
    const std::string server_address = "127.0.0.1";
    boost::asio::io_context io;
    boost::asio::ip::tcp::resolver resolver(io);
    boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(server_address, "8080");
    boost::asio::ip::tcp::socket socket(io);
    try {
        boost::asio::connect(socket, endpoints);
        std::cout << "Connected to the server!" << std::endl;
        while (true) {
        std::array<char, 128> buf;
        boost::system::error_code error;

        size_t len = socket.read_some(boost::asio::buffer(buf), error);

        if (error == boost::asio::error::eof) {
            break;
        }
        else if (error) 
            throw boost::system::system_error(error);

        // If "Connected!" is displayed, then we formed a successful TCP connection with the server.
        std::cout.write(buf.data(), len);
        std::cout.flush();
        // Let's now form the JSON to send the data to the server.
        boost::json::object registration;
        registration["username"] = ui->username_register_lineEdit->text().toStdString();
        registration["email"] = ui->email_register_lineEdit->text().toStdString();

        // Let's Hash the password to be stored in the database (this prevents us from knowing the users' passwords for their security)
        std::string password = ui->password_register_lineEdit->text().toStdString();
        std::string hash = BCrypt::generateHash(password);

        registration["hashed_password"] = hash;

        // Preparing the request...
        boost::beast::http::request<boost::beast::http::string_body> request(boost::beast::http::verb::post, "/register", 11);
        request.set(boost::beast::http::field::host, server_address);
        request.set(boost::beast::http::field::content_type, "application/json");
        request.body() = boost::json::serialize(registration);
        request.prepare_payload();

        // Let's send it!
        boost::beast::http::write(socket, request);
        }
    } catch (std::exception& e) {
        std::cout << "Connection failed: " << e.what() << std::endl;
    }

}

