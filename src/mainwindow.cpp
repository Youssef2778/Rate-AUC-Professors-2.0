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

// Stacked widget page indices:
//   0 = Login
//   1 = Register
//   2 = Leaderboard
//   3 = Homepage
//   4 = Professor

namespace beast = boost::beast;
namespace http = beast::http;

// The connection to the server is established in a background thread so the UI
// is not blocked waiting for the server to respond on startup.
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // Initialize the user pointer to nullptr at the start of the application
    user = nullptr;
    Connected = false;
    networkManager = new QNetworkAccessManager(this);
    LoginPage();

    // Attempt to establish a persistent connection in the background once the app launches
    std::thread(&MainWindow::EstablishConnection, this).detach();
}

MainWindow::~MainWindow() {
    delete ui;
}


// Clears the current user session and returns to the login screen.
void MainWindow::Logout() {
    // Clear the user session
    delete user;
    user = nullptr;
    selected_flairs.clear();
    // Return to the login page
    LoginPage();
}

// Runs in a detached background thread. Retries the connection every second
// until it succeeds. Sets Connected = true only after the server's handshake
// message is received.
void MainWindow::EstablishConnection() {
    const std::string server_address = "16.171.132.23";
    boost::asio::ip::tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(server_address, "8080");
    // continuously attempt to connect to the server until successful
    while (true) {
        try {
            boost::asio::connect(socket, endpoints);
            // Consume the 'Connected!' handshake sent by the server on accept.
            std::array<char, 128> buf;
            boost::system::error_code error;
            socket.read_some(boost::asio::buffer(buf), error);
            Connected = true;
            std::cout << "Connected to the server!"
                      << std::endl; // Confirm connection if handshake didn't throw an error
            break;                  // Connection successful, exit the loop
        } catch (std::exception &e) {
            std::cout << "Connection failed: " << e.what() << std::endl;
            socket.close();                                       // Reset the socket before retrying
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait 1 second before retrying
        }
    }
}

// Centers TargetWidget both horizontally and vertically within the given stacked
// page. Re-creates the page layout on every call because switching pages can
// leave stale layouts behind. Generated with the help of ChatGPT.
void MainWindow::CenterWidget(int pageIndex, QWidget *TargetWidget) {
    // The following lines of code are produced by ChatGPT, their purpose is to constantly center
    // the form no matter the size of the main window

    // Assume you have a widget inside the stacked page
    QWidget *page = ui->stackedWidget->widget(pageIndex); // page by index

    // Clear any existing layout on the page
    if (page->layout()) {
        delete page->layout();
    }

    QVBoxLayout *vLayout = new QVBoxLayout(page);

    // Create a horizontal layout for centering
    QHBoxLayout *hLayout = new QHBoxLayout();

    // The homepage has a fixed header bar that must stay at the top, outside
    // the centering logic.
    if (pageIndex == 3) {
        vLayout->addWidget(ui->HomePageHeader);
    }

    hLayout->addStretch();            // left spacer
    hLayout->addWidget(TargetWidget); // your target widget
    hLayout->addStretch();            // right spacer

    vLayout->addStretch(); // top spacer
    vLayout->addLayout(hLayout);
    vLayout->addStretch(); // bottom spacer

    page->setLayout(vLayout);
}

// Recursively removes and deletes all items from a layout, including nested
// sub-layouts. Necessary because Qt does not recursively clean up child layouts
// when a parent layout is deleted.
void MainWindow::ClearLayout(QLayout *layout) {
    if (!layout)
        return;

    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->layout()) {
            ClearLayout(item->layout()); // recursive cleanup
        }
        delete item->widget();
        delete item;
    }
}

// Returns true only if all four fields are non-empty, showing the appropriate
// error label for the first empty field found. Called before every network
// request on the register page to avoid sending incomplete data to the server.
bool MainWindow::presenceChecks(QString username, QString password, QString passwordTwo, QString email) {
    if (username == "") {
        ui->empty_username_error->show();
        return false;
    } else
        ui->empty_username_error->hide();

    if (password == "") {
        ui->empty_pass_error->show();
        return false;
    } else
        ui->empty_pass_error->hide();

    if (passwordTwo == "") {
        ui->empty_confPass_error->show();
        return false;
    } else
        ui->empty_confPass_error->hide();

    if (email == "") {
        ui->empty_email_error->show();
        return false;
    } else {
        ui->empty_email_error->hide();
    }

    return true;
}

void MainWindow::on_logoutButton_clicked() {
    Logout();
}

void MainWindow::on_logoutButton_2_clicked() {
    Logout();
}

void MainWindow::on_logoutButton_3_clicked() {
    Logout();
}

// This function can be called whenever we detect a connection failure to attempt reconnection
void MainWindow::Reconnect() {
    Connected = false;
    socket.close();
    std::thread(&MainWindow::EstablishConnection, this).detach();
}