#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "bcrypt/BCrypt.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    CenterWidget(0, ui->widget_1);

    // Attempt to establish a persistent connection in the background once the app launches
    std::thread(&MainWindow::EstablishConnection, this).detach();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::EstablishConnection() {
    const std::string server_address = "127.0.0.1";
    boost::asio::ip::tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(server_address, "8080");
	// continuously attempt to connect to the server until successful
    while (true) {
        try {
            boost::asio::connect(socket, endpoints);
            // Read the "Connected!" handshake once
            std::array<char, 128> buf;
            boost::system::error_code error;
            socket.read_some(boost::asio::buffer(buf), error);
            std::cout << "Connected to the server!" << std::endl; // Confirm connection if handshake didn't throw an error
            break; // Connection successful, exit the loop
        }
        catch (std::exception& e) {
            std::cout << "Connection failed: " << e.what() << std::endl;
            socket.close(); // Reset the socket before retrying
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait 1 second before retrying
        }
    }
}

void MainWindow::CenterWidget(int pageIndex, QWidget* TargetWidget) {
    // The following lines of code are produced by ChatGPT, their purpose is to constantly center the form no matter the size
    // of the main window

    // Assume you have a widget inside the stacked page
    QWidget* page = ui->stackedWidget->widget(pageIndex); // page by index
    QVBoxLayout* vLayout = new QVBoxLayout(page);

    // Create a horizontal layout for centering
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addStretch();          // left spacer
    hLayout->addWidget(TargetWidget); // your target widget
    hLayout->addStretch();          // right spacer

    vLayout->addStretch(); // top spacer
    vLayout->addLayout(hLayout);
    vLayout->addStretch(); // bottom spacer

    page->setLayout(vLayout);
}

// "Hide Password" Mechanism of the Login page
void MainWindow::on_checkBox_4_stateChanged(int arg1)
{
    if (arg1 == 2) {
        ui->lineEdit_8->setEchoMode(QLineEdit::Password);
    }
    else if (arg1 == 0) {
        ui->lineEdit_8->setEchoMode(QLineEdit::Normal);
    }
}

// Function executed when the user moves to the register page.
void MainWindow::on_register_label_4_linkActivated(const QString& link)
{
    ui->stackedWidget->setCurrentIndex(1);


    // Hiding the error messages.
    ui->empty_email_error->hide();
    ui->empty_username_error->hide();
    ui->auc_email_error->hide();
    ui->empty_pass_error->hide();
    ui->empty_confPass_error->hide();
    ui->unequal_pass_error->hide();

	CenterWidget(1, ui->widget_2);
}

// "Hide Password" Mechanism of the Register Page
void MainWindow::on_checkBox_6_stateChanged(int arg1)
{
    if (arg1 == 2) {
        ui->password_register_lineEdit->setEchoMode(QLineEdit::Password);
        ui->confPassword_register_lineEdit->setEchoMode(QLineEdit::Password);
    }
    else if (arg1 == 0) {
        ui->password_register_lineEdit->setEchoMode(QLineEdit::Normal);
        ui->confPassword_register_lineEdit->setEchoMode(QLineEdit::Normal);
    }
}

void MainWindow::on_register_label_6_linkActivated(const QString& link)
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
    else
        ui->empty_username_error->hide();

    if (ui->password_register_lineEdit->text() == "") {
        ui->empty_pass_error->show();
        return;
    }
    else
        ui->empty_pass_error->hide();

    if (ui->confPassword_register_lineEdit->text() == "") {
        ui->empty_confPass_error->show();
        return;
    }
    else
        ui->empty_confPass_error->hide();

    if (ui->email_register_lineEdit->text() == "") {
        ui->empty_email_error->show();
        return;
    }
    else {
        ui->empty_email_error->hide();

        // Checking the format of the email..
        std::string email = ui->email_register_lineEdit->text().toStdString();
        int i = email.find('@');
        if (!i)
            ui->empty_email_error->show();
        else {
            if (email.substr(i + 1, email.size() - i - 1) != "aucegypt.edu") {
                ui->auc_email_error->show();
                return;
            }
            else
                ui->auc_email_error->hide();
        }
    }

    // Checking if the two passwords input are the same..
    if (ui->password_register_lineEdit->text() != ui->confPassword_register_lineEdit->text()) {
        ui->unequal_pass_error->show();
        return;
    }
    else
        ui->unequal_pass_error->hide();

    // Now, let's see what the server thinks about the data!
    try {
        // Let's now form the JSON to send the data to the server.
        boost::json::object registration;
        registration["username"] = ui->username_register_lineEdit->text().toStdString();
        registration["email"] = ui->email_register_lineEdit->text().toStdString();

        // Let's Hash the password to be stored in the database (this prevents us from knowing the users' passwords for their security)
        std::string password = ui->password_register_lineEdit->text().toStdString();
        std::string hash = BCrypt::generateHash(password);
        registration["hashed_password"] = hash;

        // Preparing the request...
        boost::beast::http::request<boost::beast::http::string_body>
            request(boost::beast::http::verb::post, "/register", 11);
        request.set(boost::beast::http::field::host, "127.0.0.1");
        request.set(boost::beast::http::field::content_type, "application/json");
        request.body() = boost::json::serialize(registration);
        request.prepare_payload();

        // Let's send it!
        boost::beast::http::write(socket, request);

        //Send user to *student* homepage
        ui->stackedWidget->setCurrentIndex(2);
    }
    catch (std::exception& e) {
        std::cout << "Connection failed: " << e.what() << std::endl;
    }
}

//login module
void MainWindow::on_pushButton_4_clicked()
{
    //Temporary login for testing purposes
    ui->stackedWidget->setCurrentIndex(2);

    CenterWidget(2, ui->widget_3);

    //Request the departments from the server
    try {
        // Send GET /get-departments
        http::request<http::string_body> request(http::verb::get, "/get-departments", 11);
        request.set(http::field::host, "127.0.0.1");
        request.prepare_payload();
        http::write(socket, request);

        // Read the response
        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(socket, buffer, response);

        // Parse the JSON array
        auto parsed = boost::json::parse(response.body());
        boost::json::array& departments = parsed.as_array();

        for (auto& entry : departments) {
            boost::json::object& dept = entry.as_object();
            std::string name = (std::string)dept["department_name"].as_string();
            int ID = (int)dept["id"].as_int64();
            Deps[name] = ID; // Store the mapping of department name to ID
            // populate the QComboBox
            ui->DepartmentCB->addItem(QString::fromStdString(name));
        }
    }
    catch (std::exception& e) {
        std::cout << "Failed: " << e.what() << std::endl;
    }
}

void MainWindow::on_DepartmentCB_currentIndexChanged(int index)
{
    //Request the Courses from the server
    try {
        std::string DepName = ui->DepartmentCB->currentText().toStdString();
        int DepID = Deps[DepName]; // Get the department ID using the mapping stored
        // Send GET /get-courses?Id=DepID
        http::request<http::string_body> request(http::verb::get, "/get-courses?Id=" + std::to_string(DepID), 11);
        request.set(http::field::host, "127.0.0.1");
        request.prepare_payload();
        http::write(socket, request);

        // Read the response
        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(socket, buffer, response);

        // Parse the JSON array
        auto parsed = boost::json::parse(response.body());
        boost::json::array& courses = parsed.as_array();

        ui->CourseCB->clear(); // Clear previous courses before adding new ones
        for (auto& entry : courses) {
            boost::json::object& course = entry.as_object();
            std::string name = (std::string)course["course_name"].as_string();
            int ID = (int)course["id"].as_int64();
            Courses[name] = ID; // Store the mapping of course name to ID
            // populate the QComboBox
            ui->CourseCB->addItem(QString::fromStdString(name));
        }
    }
    catch (std::exception& e) {
        std::cout << "Failed: " << e.what() << std::endl;
    }
}