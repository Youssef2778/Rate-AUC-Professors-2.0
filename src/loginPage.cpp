#include "mainwindow.h"
#include "User.h"
#include "Comment.h"
#include <QProgressBar>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "ui_mainwindow.h" //removed "./" because ui_mainwindow.h is generated in build\Rate_AUC_autogen\include\ and not the build directory
#include "bcrypt/BCrypt.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

void MainWindow::LoginPage()
{
    ui->stackedWidget->setCurrentIndex(0);

    // Hiding error messages
    ui->email_notFound_error->hide();
    ui->password_incorrect_error->hide();

    CenterWidget(0, ui->widget_1);
}

// "Hide Password" Mechanism of the Login page
void MainWindow::on_checkBox_4_stateChanged(int arg1)
{
    if (arg1 == 2) {
        ui->password_login_lineEdit->setEchoMode(QLineEdit::Password);
    } else if (arg1 == 0) {
        ui->password_login_lineEdit->setEchoMode(QLineEdit::Normal);
    }
}

// Function executed when the user moves back to the login page from the register page.
void MainWindow::on_register_label_6_linkActivated(const QString& link)
{
    LoginPage();
}

// Function that gets called after clicking on the "login" button in the login page.
void MainWindow::on_pushButton_4_clicked()
{
    try{
        boost::json::object login;
        login["email"] = ui->email_login_lineEdit->text().toStdString();
        login["password"] = ui->password_login_lineEdit->text().toStdString();

        // Preparing the request...
        boost::beast::http::request<boost::beast::http::string_body> request(
            boost::beast::http::verb::post, "/login", 11);
        request.set(boost::beast::http::field::host, "127.0.0.1");
        request.set(boost::beast::http::field::content_type, "application/json");
        request.body() = boost::json::serialize(login);
        request.prepare_payload();

        // Let's send it!
        boost::beast::http::write(socket, request);

        boost::beast::flat_buffer buf;
        boost::beast::http::response<boost::beast::http::string_body> server_response;
        http::read(socket, buf, server_response);
        std::cout << "read from the server\n";
        auto parsed_response = boost::json::parse(server_response.body());
        boost::json::object json_response = parsed_response.as_object();
        bool logged_in = (bool)json_response["status"].as_bool();


        if (logged_in) {
            // Create a user for this session
            user = new User((std::string)json_response["username"].as_string(), (std::string)login["email"].as_string(),
                                (int)json_response["id"].as_int64());
            HomePage();
        } else {
            std::string error = (std::string)json_response["error"].as_string();
            if (error == "email not found")
                ui->email_notFound_error->show();
            else
                ui->email_notFound_error->hide();
            if (error == "incorrect password")
                ui->password_incorrect_error->show();
            else
                ui->password_incorrect_error->hide();
        }
    }
    catch (boost::system::system_error& e) {
    // Attempt reconnect on connection errors
    if (e.code() == boost::asio::error::eof ||
        e.code() == boost::asio::error::connection_reset ||
        e.code() == boost::asio::error::broken_pipe ||
        e.code() == boost::asio::error::not_connected ||
        e.code() == boost::beast::http::error::end_of_stream) {
        std::cout << "Connection lost, reconnecting..." << std::endl;
        Reconnect();
    } else {
        std::cout << "Network error: " << e.what() << std::endl;
    }
    }
    catch (std::exception& e) {
        // Non-network errors — don't reconnect
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Function executed when the user moves to the register page from the login page.
void MainWindow::on_register_label_4_linkActivated(const QString& link)
{
    RegisterPage();
}