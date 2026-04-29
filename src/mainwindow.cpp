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

// Homepage index = 3
// Login = 0
// Register = 1
// leaderboard = 2

namespace beast = boost::beast;
namespace http = beast::http;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Initialize the user pointer to nullptr at the start of the application
    user = nullptr;
    Connected = false;
    networkManager = new QNetworkAccessManager(this);
    LoginPage();

    // Attempt to establish a persistent connection in the background once the app launches
    std::thread(&MainWindow::EstablishConnection, this).detach();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::Logout()
{
    // Clear the user session
    delete user;
    user = nullptr;
    selected_flairs.clear();
    // Return to the login page
    LoginPage();
}

void MainWindow::EstablishConnection()
{
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
            Connected = true;
            std::cout << "Connected to the server!"
                      << std::endl;  // Confirm connection if handshake didn't throw an error
            break;                   // Connection successful, exit the loop
        } catch (std::exception& e) {
            std::cout << "Connection failed: " << e.what() << std::endl;
            socket.close();  // Reset the socket before retrying
            std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait 1 second before retrying
        }
    }
}

void MainWindow::CenterWidget(int pageIndex, QWidget* TargetWidget)
{
    // The following lines of code are produced by ChatGPT, their purpose is to constantly center
    // the form no matter the size of the main window

    // Assume you have a widget inside the stacked page
    QWidget* page = ui->stackedWidget->widget(pageIndex);  // page by index
    QVBoxLayout* vLayout = new QVBoxLayout(page);

    // Create a horizontal layout for centering
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addStretch();             // left spacer
    hLayout->addWidget(TargetWidget);  // your target widget
    hLayout->addStretch();             // right spacer

    vLayout->addStretch();  // top spacer
    vLayout->addLayout(hLayout);
    vLayout->addStretch();  // bottom spacer

    page->setLayout(vLayout);
}




void MainWindow::ClearLayout(QLayout* layout) {
    if (!layout) return;

    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->layout()) {
            ClearLayout(item->layout()); // recursive cleanup
        }
        delete item->widget();
        delete item;
    }
}
