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

enum flairID { null, recommend, avoid, meh, easyA, fastPaced, highWorkLoad };


// Loads the professor page: fetches comments for the current professor/course
// pair, renders them, then triggers an AI summary fetch.
void MainWindow::professorPage() {
        if (!Connected) {
        ConnectionFailedPopup();
        return;
    }
    selected_flairs.clear();
    ui->stackedWidget->setCurrentIndex(4);
    ui->scrollArea->setWidgetResizable(true);
    ui->widget_7->setMaximumHeight(QWIDGETSIZE_MAX);

    ui->usernameLabel_2->setText(QString::fromStdString(user->GetUsername()));
    ui->emailLabel_2->setText(QString::fromStdString(user->GetEmail()));

    ui->professorName->setText("Dr. " + QString::fromStdString(Profs[user->GetCurrentProfID()]));
    ui->courseName->setText(QString::fromStdString(Courses[user->GetCurrentCourseID()]));


    // Request the comments for this professor-course from the server
    try {
        // Send GET /get-comments?CourseId=CourseID&ProfId=ProfID
        http::request<http::string_body> request(
            http::verb::get,
            "/get-comments?CourseId=" + std::to_string(user->GetCurrentCourseID()) +
                "&ProfId=" + std::to_string(user->GetCurrentProfID()),
            11);
        request.set(http::field::host, "127.0.0.1");
        request.prepare_payload();
        http::write(socket, request);

        // Read the response
        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(socket, buffer, response);

        // Parse the JSON array
        auto parsed = boost::json::parse(response.body());
        boost::json::array &comments = parsed.as_array();

        // clear comments before populating it with the new comments
        Comments.clear();
        ClearLayout(ui->widget_7->layout());


        for (auto &entry : comments) {
            boost::json::object &comment = entry.as_object();
            vector<int> flairs;
            auto &flairs_val = comment["flairs"];
            // Flairs are stored in the database as a JSON array serialized to a
            // string (e.g. '[1,3]'). An empty string means no flairs were selected.
            if (flairs_val != "") {
                boost::json::array flairs_arr;
                flairs_arr = boost::json::parse((std::string)flairs_val.as_string()).as_array();
                std::cout << "Parsed string flairs into array: " << flairs_arr << std::endl;
                for (auto &flair : flairs_arr) {
                    flairs.push_back((int)flair.as_int64());
                }
            }

            Comments.push_back(Comment(
                (int)comment["id"].as_int64(),
                (int)comment["user_id"].as_int64(),
                (std::string)comment["username"].as_string(),
                (std::string)comment["content"].as_string(),
                (std::string)comment["timestamp"].as_string(),
                flairs));
        }
    } catch (boost::system::system_error &e) {
        // Attempt reconnect on connection errors
        if (e.code() == boost::asio::error::eof || e.code() == boost::asio::error::connection_reset ||
            e.code() == boost::asio::error::broken_pipe || e.code() == boost::asio::error::not_connected ||
            e.code() == boost::beast::http::error::end_of_stream) {
            std::cout << "Connection lost, reconnecting..." << std::endl;
            Connected = false; // Set to false to prevent multiple simultaneous reconnect attempts
            Reconnect();
        } else {
            std::cout << "Network error: " << e.what() << std::endl;
        }
    } catch (std::exception &e) {
        // Non-network errors — don't reconnect
        std::cout << "Error: " << e.what() << std::endl;
    }
    cout << "Comments for Professor " << Profs[user->GetCurrentProfID()] << " and Course "
         << Courses[user->GetCurrentCourseID()] << " loaded: " << Comments.size() << endl;
    DisplayComments();

    // Fetch AI summary after comments are displayed so the page isn't blocked
    // waiting for the Groq API response.
    std::string courseIdStr = std::to_string(user->GetCurrentCourseID());
    std::string profIdStr = std::to_string(user->GetCurrentProfID());
    fetchAiSummary(courseIdStr, profIdStr);
    // ==========================================
}


// Builds and inserts a styled comment card into the comments scroll area. Cards
// are inserted at index 0 so the most recently posted comment appears at the top.
void MainWindow::CreateComment(Comment comment) {
    QWidget *card = new QWidget();
    card->setObjectName("commentCard");
    card->setMinimumHeight(100);
    card->setMinimumWidth(643);
    card->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    card->setStyleSheet("QWidget#commentCard {"
                        "   background-color: rgba(255, 255, 255, 0.12);"
                        "   border: 1px solid rgba(255, 255, 255, 0.25);"
                        "   border-radius: 16px;"
                        "   margin: 7px 4px;"
                        "   padding: 3px;"
                        "}");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(card);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(10);

    QHBoxLayout *username_flair = new QHBoxLayout;

    QLabel *usernameLabel = new QLabel(QString::fromStdString(comment.name));
    usernameLabel->setStyleSheet("font-family: 'Segoe UI';"
                                 "font-size: 16px;"
                                 "font-weight: bold;"
                                 "color: rgba(255, 255, 255, 0.95);"
                                 "background: transparent;"
                                 "border: none;");

    username_flair->addWidget(usernameLabel);

    QLabel *contentLabel = new QLabel(QString::fromStdString(comment.Content));
    contentLabel->setWordWrap(true);
    contentLabel->setMinimumHeight(40);
    contentLabel->setStyleSheet("font-family: 'Segoe UI';"
                                "font-size: 15px;"
                                "color: rgba(255, 255, 255, 0.85);"
                                "background: transparent;"
                                "border: none;");

    QHBoxLayout *bottomLayout = new QHBoxLayout();

    QLabel *datetimeLabel = new QLabel(QString::fromStdString(comment.PrintTime()));
    datetimeLabel->setStyleSheet("font-family: 'Segoe UI';"
                                 "font-size: 12px;"
                                 "color: rgba(255, 255, 255, 0.45);"
                                 "background: transparent;"
                                 "border: none;");

    // Render each flair as a colored badge. The null flair is hidden — it acts
    // as a default/placeholder value when no flair was selected.
    if (comment.flairs.size() > 0) {
        for (int i = 0; i < comment.flairs.size(); i++) {
            QPushButton *flair = new QPushButton;
            flair->setCheckable(false);
            flair->setStyleSheet("color: white;"
                                 "border: 1.5px solid rgba(0, 0,0 , 0.3);"
                                 "border-radius: 7px;"
                                 "padding: 2px 8px;"
                                 "font-size: 11px;"
                                 "font-weight: bold;"
                                 "min-height: 18px;");

            QString current_stylesheet = flair->styleSheet();

            switch (comment.flairs[i]) {
            case null:
                flair->hide();
                break;
            case recommend:
                flair->setStyleSheet(current_stylesheet + "background-color: rgba(0, 200, 0, 1);");
                flair->setText("recommend");
                break;
            case avoid:
                flair->setStyleSheet(current_stylesheet + "background-color: rgba(200, 0, 0, 1);");
                flair->setText("avoid");
                break;
            case meh:
                flair->setStyleSheet(current_stylesheet + "background-color: rgba(135, 206, 235, 1);");
                flair->setText("meh");
                break;
            case easyA:
                flair->setStyleSheet(current_stylesheet + "background-color: rgba(0, 180, 0, 1);");
                flair->setText("easyA");
                break;
            case fastPaced:
                flair->setStyleSheet(current_stylesheet + "background-color: rgba(255, 140, 0, 1);");
                flair->setText("fastPaced");
                break;
            case highWorkLoad:
                flair->setStyleSheet(current_stylesheet + "background-color: rgba(200, 0, 0, 1);");
                flair->setText("highWorkLoad");
                break;
            }
            bottomLayout->addWidget(flair);
        }
    }

    bottomLayout->addStretch();
    bottomLayout->addWidget(datetimeLabel);


    mainLayout->addLayout(username_flair);
    mainLayout->addWidget(contentLabel);
    mainLayout->addLayout(bottomLayout);

    // Add card to the top of the comments section
    QBoxLayout *layout = qobject_cast<QBoxLayout *>(ui->widget_7->layout()); // cast as QBoxLayout to use insertWidget
    if (layout)
        layout->insertWidget(0, card);
}
// Renders all loaded comments into the scroll area.
void MainWindow::DisplayComments() {
    for (Comment comment : Comments) {
        CreateComment(comment);
    }
}


// Handles the post comment button. Builds a comment JSON payload including the
// selected flair IDs and sends it to the server. On success, the server returns
// the generated comment ID and timestamp, which are used to construct the local
// Comment object so the UI can be updated without a second fetch.
void MainWindow::on_postComment_clicked() {
    if (!Connected) {
        ConnectionFailedPopup();
        return;
    }
    std::string ProfName = ui->professorName->text().toStdString().substr(4); // Remove "Dr.  " prefix
    std::string CourseName = ui->courseName->text().toStdString();
    std::string content = ui->commentLineEdit->text().toStdString();
    if (content.empty())
        return; // Don't post empty comments
    try {
        // Let's now form the JSON to send the data to the server.
        boost::json::object comment;
        int UserID = user->GetID();

        boost::json::array flairs;
        for (const auto &flair : selected_flairs) {
            flairs.push_back(flair);
            std::cout << "Selected flair added to JSON: " << flair << endl;
        }


        comment["user_id"] = UserID;
        comment["content"] = content;
        comment["prof_id"] = user->GetCurrentProfID();
        comment["course_id"] = user->GetCurrentCourseID();
        comment["flairs"] = flairs;


        // Preparing the request...
        boost::beast::http::request<boost::beast::http::string_body> request(
            boost::beast::http::verb::post, "/comment", 11);
        request.set(boost::beast::http::field::host, "127.0.0.1");
        request.set(boost::beast::http::field::content_type, "application/json");
        request.body() = boost::json::serialize(comment);
        request.prepare_payload();

        // Let's send it!
        boost::beast::http::write(socket, request);


        // Retrieve the generated comment ID & timestamp
        boost::beast::flat_buffer buf;
        boost::beast::http::response<boost::beast::http::string_body> server_response;
        boost::beast::http::read(socket, buf, server_response);
        auto parsed_response = boost::json::parse(server_response.body());
        boost::json::object response = parsed_response.as_object();

        std::string timestamp = (std::string)response["timestamp"].as_string();
        int commentID = (int)response["id"].as_int64();

        cout << "Comment posted with ID: " << commentID << " at " << timestamp << endl;

        // Create comment object
        Comment commentObj(commentID, UserID, user->GetUsername(), content, timestamp, selected_flairs);
        Comments.push_back(commentObj);


        // Display the new comment
        CreateComment(commentObj);

    } catch (boost::system::system_error &e) {
        // Attempt reconnect on connection errors
        if (e.code() == boost::asio::error::eof || e.code() == boost::asio::error::connection_reset ||
            e.code() == boost::asio::error::broken_pipe || e.code() == boost::asio::error::not_connected ||
            e.code() == boost::beast::http::error::end_of_stream) {
            std::cout << "Connection lost, reconnecting..." << std::endl;
            Connected = false; // Set to false to prevent multiple simultaneous reconnect attempts
            Reconnect();
        } else {
            std::cout << "Network error: " << e.what() << std::endl;
        }
    } catch (std::exception &e) {
        // Non-network errors — don't reconnect
        std::cout << "Error: " << e.what() << std::endl;
    }
    ui->commentLineEdit->clear(); // Clear the input field after posting
    ui->mehFlair->setChecked(false);
    ui->avoidFlair->setChecked(false);
    ui->recommendFlair->setChecked(false);
    ui->easyAFlair->setChecked(false);
    ui->highWorkLoadFlair->setChecked(false);
    ui->fastPacedFlair->setChecked(false);
    selected_flairs.clear();
}

// Fetches an AI-generated summary of the professor's comments from the server,
// which proxies the request to the Groq API. processEvents() is called before
// the blocking HTTP read so the 'Generating...' label is visible during the wait.
void MainWindow::fetchAiSummary(std::string courseId, std::string profId) {
    
    // 1. Update the UI
    ui->summaryLabel->setText("🤖 Generating AI Summary... Please wait.");

    // Force Qt to update the screen immediately before the server request blocks it
    QCoreApplication::processEvents();

    try {
        // 2. Build the HTTP request using your existing Boost setup
        std::string target = "/get-summary?CourseId=" + courseId + "&ProfId=" + profId;
        http::request<http::string_body> request(http::verb::get, target, 11);
        request.set(http::field::host, "127.0.0.1");
        request.prepare_payload();

        // 3. Send the request down your existing persistent socket!
        http::write(socket, request);

        // 4. Read the server's response
        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(socket, buffer, response);

        // 5. Parse the JSON and display it
        if (response.result() == http::status::ok) {
            auto parsed = boost::json::parse(response.body());
            boost::json::object jsonObj = parsed.as_object();

            std::string aiSummary = (std::string)jsonObj["summary"].as_string();
            ui->summaryLabel->setText(QString::fromStdString(aiSummary));
        } else {
            ui->summaryLabel->setText("Failed to load AI summary. (Server error)");
        }
    } catch (boost::system::system_error &e) {
        // Attempt reconnect on connection errors
        if (e.code() == boost::asio::error::eof || e.code() == boost::asio::error::connection_reset ||
            e.code() == boost::asio::error::broken_pipe || e.code() == boost::asio::error::not_connected ||
            e.code() == boost::beast::http::error::end_of_stream) {
            std::cout << "Connection lost, reconnecting..." << std::endl;
            Connected = false; // Set to false to prevent multiple simultaneous reconnect attempts
            Reconnect();
        } else {
            std::cout << "Network error: " << e.what() << std::endl;
        }
    } catch (std::exception &e) {
        // Non-network errors — don't reconnect
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Each flair handler uses the erase-remove idiom to remove the flair ID from
// selected_flairs when unchecked, and push_back when checked. This ensures no
// duplicates and handles toggling correctly.
void MainWindow::on_mehFlair_clicked(bool checked) {
    std::cout << "Meh flair button clicked." << std::endl;
    if (!checked) {
        selected_flairs.erase(std::remove(selected_flairs.begin(), selected_flairs.end(), meh), selected_flairs.end());
        std::cout << "Meh flair removed." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    } else {
        selected_flairs.push_back(meh);
        std::cout << "Meh flair added." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    }
}

void MainWindow::on_avoidFlair_clicked(bool checked) {
    std::cout << "Avoid flair button clicked." << std::endl;
    if (!checked) {
        selected_flairs.erase(
            std::remove(selected_flairs.begin(), selected_flairs.end(), avoid), selected_flairs.end());
        std::cout << "Avoid flair removed." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    } else {
        selected_flairs.push_back(avoid);
        std::cout << "Avoid flair added." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    }
}

void MainWindow::on_recommendFlair_clicked(bool checked) {
    std::cout << "Recommend flair button clicked." << std::endl;
    if (!checked) {
        selected_flairs.erase(
            std::remove(selected_flairs.begin(), selected_flairs.end(), recommend), selected_flairs.end());
        std::cout << "Recommend flair removed." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    } else {
        selected_flairs.push_back(recommend);
        std::cout << "Recommend flair added." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    }
}

void MainWindow::on_easyAFlair_clicked(bool checked) {
    std::cout << "Easy A flair button clicked." << std::endl;
    if (!checked) {
        selected_flairs.erase(
            std::remove(selected_flairs.begin(), selected_flairs.end(), easyA), selected_flairs.end());
        std::cout << "Easy A flair removed." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    } else {
        selected_flairs.push_back(easyA);
        std::cout << "Easy A flair added." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    }
}

void MainWindow::on_highWorkLoadFlair_clicked(bool checked) {
    std::cout << "High Workload flair button clicked." << std::endl;
    if (!checked) {
        selected_flairs.erase(
            std::remove(selected_flairs.begin(), selected_flairs.end(), highWorkLoad), selected_flairs.end());
        std::cout << "High Workload flair removed." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    } else {
        selected_flairs.push_back(highWorkLoad);
        std::cout << "High Workload flair added." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    }
}

void MainWindow::on_fastPacedFlair_clicked(bool checked) {
    std::cout << "Fast Paced flair button clicked." << std::endl;
    if (!checked) {
        selected_flairs.erase(
            std::remove(selected_flairs.begin(), selected_flairs.end(), fastPaced), selected_flairs.end());
        std::cout << "Fast Paced flair removed." << std::endl;
        std::cout << "Current selected flairs: ";
        for (const auto &flair : selected_flairs) {
            std::cout << flair << " ";
        }
        std::cout << std::endl;
    } else {
        selected_flairs.push_back(fastPaced);
        std::cout << "Fast Paced flair added." << std::endl;
        std::cout << "Current selected flairs: ";
    }
}

// Resets the current professor ID before navigating back so stale prof state
// doesn't bleed into the next professor page visit.
void MainWindow::on_BackToLeaderboard_clicked() {
    user->SetCurrentProfID(-1);
    LeaderboardPage();
}
