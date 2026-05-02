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
#include <regex>
#include "helpers.hpp"

#include "ui_mainwindow.h" //removed "./" because ui_mainwindow.h is generated in build\Rate_AUC_autogen\include\ and not the build directory
#include "bcrypt/BCrypt.hpp"

namespace beast = boost::beast;
namespace http = beast::http;


// Navigates to the register page and resets all error labels to hidden.
void MainWindow::RegisterPage() {
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

// arg1 == 2 corresponds to Qt::Checked; arg1 == 0 corresponds to Qt::Unchecked.
// Both password fields are toggled together so they stay in sync.
void MainWindow::on_checkBox_6_stateChanged(int arg1) {
    if (arg1 == 2) {
        ui->password_register_lineEdit->setEchoMode(QLineEdit::Password);
        ui->confPassword_register_lineEdit->setEchoMode(QLineEdit::Password);
    } else if (arg1 == 0) {
        ui->password_register_lineEdit->setEchoMode(QLineEdit::Normal);
        ui->confPassword_register_lineEdit->setEchoMode(QLineEdit::Normal);
    }
}


// Handles the Register button click. Runs client-side validation in this order:
//   1. Presence checks (all fields non-empty)
//   2. Email format check (via validEmail regex)
//   3. AUC domain check (must be @aucegypt.edu)
//   4. Password confirmation match.
// Only sends the request if all checks pass. The password is hashed client-side
// with BCrypt before transmission so the server never sees the plain-text password.
void MainWindow::on_pushButton_6_clicked() {
    // Client-side validation (presence check + format check of email)
    QString username = ui->username_register_lineEdit->text();
    QString password = ui->password_register_lineEdit->text();
    QString confPassword = ui->confPassword_register_lineEdit->text();
    QString email = ui->email_register_lineEdit->text();

    if (!presenceChecks(username, password, confPassword, email))
        return;

    // Validate email format, then enforce AUC-specific domain restriction.
    int i = email.toStdString().find('@');
    if (!validEmail(email.toStdString())) {
        ui->empty_email_error->show();
        return;
    } else {
        if (email.toStdString().substr(i + 1, email.size() - i - 1) != "aucegypt.edu") {
            ui->auc_email_error->show();
            return;
        } else
            ui->auc_email_error->hide();
    }


    // Checking if the two passwords input are the same..
    if (ui->password_register_lineEdit->text() != ui->confPassword_register_lineEdit->text()) {
        ui->unequal_pass_error->show();
        return;
    } else
        ui->unequal_pass_error->hide();

    // Now, let's see what the server thinks about the data!
    try {
        if (Connected == false)
            return;
        // Let's now form the JSON to send the data to the server.
        boost::json::object registration;
        registration["username"] = ui->username_register_lineEdit->text().toStdString();
        registration["email"] = ui->email_register_lineEdit->text().toStdString();

        // Hash the password before sending — the server stores only the hash, so
        // even the backend never knows the user's actual password.
        std::string password = ui->password_register_lineEdit->text().toStdString();
        std::string hash = BCrypt::generateHash(password);
        registration["hashed_password"] = hash;

        // Preparing the request...
        boost::beast::http::request<boost::beast::http::string_body> request(
            boost::beast::http::verb::post, "/register", 11);
        request.set(boost::beast::http::field::host, "127.0.0.1");
        request.set(boost::beast::http::field::content_type, "application/json");
        request.body() = boost::json::serialize(registration);
        request.prepare_payload();

        // Let's send it!
        boost::beast::http::write(socket, request);

        // The server responds with the auto-generated user ID so we can construct
        // the User object without a second round-trip.
        boost::beast::flat_buffer buf;
        boost::beast::http::response<boost::beast::http::string_body> server_response;
        boost::beast::http::read(socket, buf, server_response);
        auto parsed_response = boost::json::parse(server_response.body());
        boost::json::object response = parsed_response.as_object();

        // Create user for this session
        user = new User(
            (std::string)registration["username"].as_string(),
            (std::string)registration["email"].as_string(),
            (int)response["id"].as_int64());

        // Send user to homepage
        HomePage();
    } catch (boost::system::system_error &e) {
        // Attempt reconnect on connection errors
        if (e.code() == boost::asio::error::eof || e.code() == boost::asio::error::connection_reset ||
            e.code() == boost::asio::error::broken_pipe || e.code() == boost::asio::error::not_connected ||
            e.code() == boost::beast::http::error::end_of_stream) {
            std::cout << "Connection lost, reconnecting..." << std::endl;
            Reconnect();
        } else {
            std::cout << "Network error: " << e.what() << std::endl;
        }
    } catch (std::exception &e) {
        // Non-network errors — don't reconnect
        std::cout << "Error: " << e.what() << std::endl;
    }
}
