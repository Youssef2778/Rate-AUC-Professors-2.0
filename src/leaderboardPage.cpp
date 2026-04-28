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


void MainWindow::LeaderboardPage(){

    ui->stackedWidget->setCurrentIndex(2);

            // ==========================================
            // 1. LAYOUT & BACK BUTTON FIX
            // ==========================================
    QWidget* page = ui->stackedWidget->widget(2);
    if (page->layout()) {
        delete page->layout();
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(page);

    // Add some padding around the edges of the screen so it breathes
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15); // Adds a gap between the top bar and the table

    QHBoxLayout *topBarLayout = new QHBoxLayout();
    topBarLayout->addStretch(); // Pushes the button to the right

            // Style the Back Button so it looks good!
    ui->backButton->setText("⬅ Back to Courses");
    ui->backButton->setCursor(Qt::PointingHandCursor);
    ui->backButton->setStyleSheet(
        "QPushButton { "
        "background-color: #1d8e9e; "
        "color: white; "
        "border-radius: 5px; "
        "font-size: 16px; "
        "font-weight: bold; "
        "padding: 8px 20px; "
        "} "
        "QPushButton:hover { background-color: #0b2239; border: 2px solid #1d8e9e; }"
        );

    topBarLayout->addWidget(ui->backButton);

    mainLayout->addLayout(topBarLayout);
    mainLayout->addWidget(ui->tableWidget);

    page->setLayout(mainLayout);

    // ==========================================
    // 2. TABLE COLUMN SIZES FIX (BALANCED)
    // ==========================================
    QHeaderView* header = ui->tableWidget->horizontalHeader();

            // Stop the last column from forcing a stretch
    header->setStretchLastSection(false);

            // Rank: Set to a fixed, wider size so it takes up more visual space
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableWidget->setColumnWidth(0, 150); // 150 pixels wide

    // Name: Still stretches, but now has less space to steal
    header->setSectionResizeMode(1, QHeaderView::Stretch);

    // Score: Set to a fixed, wider size
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->tableWidget->setColumnWidth(2, 150); // 150 pixels wide

            // Up & Down Buttons: Locked to 80 pixels
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableWidget->setColumnWidth(3, 80);

    header->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->tableWidget->setColumnWidth(4, 80);

            // Style the Header row
    header->setStyleSheet(
        "QHeaderView::section { "
        "font-size: 18px; "
        "font-weight: bold; "
        "background-color: #061524; "
        "color: white; "
        "border: none; "
        "border-bottom: 2px solid #1d8e9e; "
        "}"
        );

    this->refreshList();
}


void MainWindow::handleUpvote(const std::string& profID, int courseID, std::string CourseName) {
    try {
        boost::json::object voteData;
        // FIX 1 & 2: Correct spelling AND convert the string to an integer!
        voteData["professor_id"] = std::stoi(profID);
        voteData["course_id"] = courseID;
        voteData["user_id"] = user->GetID();
        voteData["vote"] = 1; // 1 means Upvote

        http::request<http::string_body> request(http::verb::post, "/upvote", 11);
        request.set(http::field::content_type, "application/json");
        request.body() = boost::json::serialize(voteData);
        request.prepare_payload();

        http::write(socket, request);

        // Read response to keep the socket clean
        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(socket, buffer, response);

        if (response.result() == http::status::ok) {
            std::cout << "Upvote success!" << std::endl;
            // REFRESH the screen immediately
            this->refreshList();
        }
    } catch (std::exception& e) {
        std::cout << "Upvote failed: " << e.what() << std::endl;
    }
}

void MainWindow::refreshList() {
    try {
        // 1. Ask for professors using the SAVED ID
        std::string target = "/get-professors?Id=" + std::to_string(user->GetCurrentCourseID());
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, "127.0.0.1");
        http::write(socket, req);

        // 2. Read the response
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(socket, buffer, res);

        // 3. Parse and Clear Table
        auto professors = boost::json::parse(res.body()).as_array();
        ui->tableWidget->setRowCount(0); // Clear old rows

        int row = 0;
        for (auto& entry : professors) {
            auto& prof = entry.as_object();
            ui->tableWidget->insertRow(row);

            // Data Extraction
            std::string nameStr = (std::string) prof.at("name").as_string();
            std::string idStr = std::to_string(prof.at("id").as_int64());
            std::string scoreStr = (std::string) prof.at("score").as_string();
            Profs[atoi(idStr.c_str())] = nameStr;  // Store the mapping of professor name to ID

            // Create Items
            QTableWidgetItem *rank = new QTableWidgetItem(QString::number(row + 1));
            QPushButton* name = new QPushButton(QString::fromStdString(nameStr));
            name->setFlat(true);
            name->setCursor(Qt::PointingHandCursor);
            name->setStyleSheet(
                "QPushButton { color: white; text-decoration: underline; background: transparent; border: none; }"
                "QPushButton:hover { color: rgba(255, 255, 255, 0.6); }"
            );
            QObject::connect(name, &QPushButton::clicked, this, [this, idStr]() {
                user->SetCurrentProfID(atoi(idStr.c_str()));
                professorPage();
            });

            QTableWidgetItem *score = new QTableWidgetItem(QString::fromStdString(scoreStr));

            // --- ADD THIS TO MAKE THE FONT BIGGER ---
            QFont tableFont;
            tableFont.setPointSize(16); // Nice, big, readable font
            tableFont.setBold(true);

            rank->setFont(tableFont);
            name->setFont(tableFont);
            score->setFont(tableFont);
            // ----------------------------------------

            rank->setTextAlignment(Qt::AlignCenter);
            // name->setTextAlignment(Qt::AlignCenter);
            score->setTextAlignment(Qt::AlignCenter);

            ui->tableWidget->setItem(row, 0, rank);
            ui->tableWidget->setCellWidget(row, 1, name);
            ui->tableWidget->setItem(row, 2, score);
            QPushButton *up = new QPushButton;
            QPushButton *down = new QPushButton;

            up->setIcon(QIcon(":/images/up.png"));   // Use your exact resource path
            down->setIcon(QIcon(":/images/down.png")); // Use your exact resource path

            // Make the icons look good
            up->setIconSize(QSize(32, 32));
            down->setIconSize(QSize(32, 32));
            QString btnStyle = "QPushButton { "
                "background-color: #0b2239; "
                "border: 1px solid #1d8e9e; "
                "border-radius: 8px; "
                "} "
                "QPushButton:hover { background-color: #1d8e9e; } "
                "QPushButton:pressed { background-color: #091a2b; }";

            up->setStyleSheet(btnStyle);
            down->setStyleSheet(btnStyle);
            // Re-capture the current Course ID and Prof ID for the lambda
            int currentCID = user->GetCurrentCourseID();
            connect(up, &QPushButton::clicked, this, [this, idStr, currentCID]() {
                handleUpvote(idStr, currentCID, Courses[user->GetCurrentCourseID()]);
            });

            connect(down, &QPushButton::clicked, this, [this, idStr, currentCID]() {
                handleDownvote(idStr, currentCID, Courses[user->GetCurrentCourseID()]);
            });

            ui->tableWidget->setCellWidget(row, 3, up);
            ui->tableWidget->setCellWidget(row, 4, down);
            ui->tableWidget->setRowHeight(row, 60);
            row++;
        }
    } catch (std::exception& e) {
        std::cerr << "Refresh failed: " << e.what() << std::endl;
    }
}

void MainWindow::handleDownvote(const std::string& profID, int courseID, std::string CourseName) {
    try {
        boost::json::object voteData;
        // FIX 2: Convert the string to an integer!
        voteData["professor_id"] = std::stoi(profID);
        voteData["course_id"] = courseID;
        voteData["user_id"] = user->GetID();
        voteData["vote"] = -1; // -1 means Downvote

        http::request<http::string_body> request(http::verb::post, "/downvote", 11);
        request.set(http::field::content_type, "application/json");
        request.body() = boost::json::serialize(voteData);
        request.prepare_payload();

                // 1. Send the vote
        http::write(socket, request);

                // 2. Read the response (Must do this to clear the socket!)
        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(socket, buffer, response);

                // 3. Check if successful and REFRESH
        if (response.result() == http::status::ok) {
            std::cout << "Downvote success! Refreshing..." << std::endl;
            this->refreshList();
        }
    } catch (std::exception& e) {
        std::cout << "Downvote failed: " << e.what() << std::endl;
    }
}


void MainWindow::on_backButton_clicked()
{
    // 1. Go back to the Course Selection page (Index 3 based on your earlier code)
    HomePage();
}

