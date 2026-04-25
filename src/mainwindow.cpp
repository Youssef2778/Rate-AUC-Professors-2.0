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
    
    LoginPage();

    // Attempt to establish a persistent connection in the background once the app launches
    std::thread(&MainWindow::EstablishConnection, this).detach();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::LoginPage()
{
    ui->stackedWidget->setCurrentIndex(0);

    // Hiding error messages
    ui->email_notFound_error->hide();
    ui->password_incorrect_error->hide();

    CenterWidget(0, ui->widget_1);
}

void MainWindow::RegisterPage()
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

// Loads the homepage when called.
void MainWindow::HomePage()
{
    ui->stackedWidget->setCurrentIndex(3);
    CenterWidget(3, ui->widget_3);

    // Request the departments from the server
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
            Deps[name] = ID;  // Store the mapping of department name to ID
            // populate the QComboBox
            ui->DepartmentCB->addItem(QString::fromStdString(name));
        }
    } catch (std::exception& e) {
        std::cout << "Failed: " << e.what() << std::endl;
    }
}


void MainWindow::professorPage(std::string ProfName, std::string CourseName) {
    ui->stackedWidget->setCurrentIndex(4);
    ui->scrollArea->setWidgetResizable(true);
    ui->widget_7->setMaximumHeight(QWIDGETSIZE_MAX);

	ui->professorName->setText("Dr. " + QString::fromStdString(ProfName));
	ui->courseName->setText(QString::fromStdString(CourseName));


	// Request the comments for this professor-course from the server
    try {

        // Send GET /get-comments?CourseId=CourseID&ProfId=ProfID
        http::request<http::string_body> request(http::verb::get,
            "/get-comments?CourseId=" + std::to_string(Courses[CourseName]) + "&ProfId=" + std::to_string(Profs[ProfName]), 11);
        request.set(http::field::host, "127.0.0.1");
        request.prepare_payload();
        http::write(socket, request);

        // Read the response
        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(socket, buffer, response);

        // Parse the JSON array
        auto parsed = boost::json::parse(response.body());
        boost::json::array& comments = parsed.as_array();

        // clear comments before populating it with the new comments
        Comments.clear();
        ClearLayout(ui->widget_7->layout());

        for (auto& entry : comments) {
            boost::json::object& comment = entry.as_object();
            Comments.push_back(Comment( (int)comment["id"].as_int64(), (int)comment["user_id"].as_int64(), (std::string)comment["username"].as_string(),
                                       (std::string)comment["content"].as_string(), (std::string)comment["timestamp"].as_string() ));
        }
    }
    catch (std::exception& e) {
        std::cout << "Failed: " << e.what() << std::endl;
    }
	cout << "Comments for Professor " << ProfName << " and Course " << CourseName << " loaded: " << Comments.size() << endl;
    DisplayComments();
	cout << "Comments displayed" << endl;
}

void MainWindow::Logout()
{
    // Clear the user session
    delete user;
    user = nullptr;
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

// "Hide Password" Mechanism of the Login page
void MainWindow::on_checkBox_4_stateChanged(int arg1)
{
    if (arg1 == 2) {
        ui->password_login_lineEdit->setEchoMode(QLineEdit::Password);
    } else if (arg1 == 0) {
        ui->password_login_lineEdit->setEchoMode(QLineEdit::Normal);
    }
}

// Function executed when the user moves to the register page.
void MainWindow::on_register_label_4_linkActivated(const QString& link)
{
    RegisterPage();
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

// Function executed when the user moves back to the login page from the register page.
void MainWindow::on_register_label_6_linkActivated(const QString& link)
{
    LoginPage();
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
    } else
        ui->empty_username_error->hide();

    if (ui->password_register_lineEdit->text() == "") {
        ui->empty_pass_error->show();
        return;
    } else
        ui->empty_pass_error->hide();

    if (ui->confPassword_register_lineEdit->text() == "") {
        ui->empty_confPass_error->show();
        return;
    } else
        ui->empty_confPass_error->hide();

    if (ui->email_register_lineEdit->text() == "") {
        ui->empty_email_error->show();
        return;
    } else {
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
            } else
                ui->auc_email_error->hide();
        }
    }

    // Checking if the two passwords input are the same..
    if (ui->password_register_lineEdit->text() != ui->confPassword_register_lineEdit->text()) {
        ui->unequal_pass_error->show();
        return;
    } else
        ui->unequal_pass_error->hide();

    // Now, let's see what the server thinks about the data!
    try {
        // Let's now form the JSON to send the data to the server.
        boost::json::object registration;
        registration["username"] = ui->username_register_lineEdit->text().toStdString();
        registration["email"] = ui->email_register_lineEdit->text().toStdString();

        // Let's Hash the password to be stored in the database (this prevents us from knowing the
        // users' passwords for their security)
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

        // Retrieve the generated user ID
        boost::beast::flat_buffer buf;
        boost::beast::http::response<boost::beast::http::string_body> server_response;
        boost::beast::http::read(socket, buf, server_response);
        auto parsed_response = boost::json::parse(server_response.body());
        boost::json::object response = parsed_response.as_object();
        
        // Create user for this session
        user = new User((std::string)registration["username"].as_string(), (std::string)registration["email"].as_string(),
                              (int)response["id"].as_int64());

        // Send user to homepage
        HomePage();
    } catch (std::exception& e) {
        std::cout << "Connection failed: " << e.what() << std::endl;
    }
}

// Function that gets called after clicking on the "login" button in the login page.
void MainWindow::on_pushButton_4_clicked()
{
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

void MainWindow::on_DepartmentCB_currentIndexChanged(int index)
{
    // Request the Courses from the server
    try {
        std::string DepName = ui->DepartmentCB->currentText().toStdString();
        int DepID = Deps[DepName];  // Get the department ID using the mapping stored
        // Send GET /get-courses?Id=DepID
        http::request<http::string_body> request(http::verb::get,
                                                 "/get-courses?Id=" + std::to_string(DepID), 11);
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

        ui->CourseCB->clear();  // Clear previous courses before adding new ones
        for (auto& entry : courses) {
            boost::json::object& course = entry.as_object();
            std::string name = (std::string)course["course_name"].as_string();
            int ID = (int)course["id"].as_int64();
            Courses[name] = ID;  // Store the mapping of course name to ID
            // populate the QComboBox
            ui->CourseCB->addItem(QString::fromStdString(name));
        }
    } catch (std::exception& e) {
        std::cout << "Failed: " << e.what() << std::endl;
    }
}
void MainWindow::on_pushButton_clicked()
{
    std::string CourseName = ui->CourseCB->currentText().toStdString();
    this->m_currentCourseId = Courses[CourseName];

    ui->stackedWidget->setCurrentIndex(2);
    this->showMaximized();

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

    this->refreshList(CourseName);
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
            this->refreshList(CourseName);
        }
    } catch (std::exception& e) {
        std::cout << "Upvote failed: " << e.what() << std::endl;
    }
}
void MainWindow::refreshList(std::string CourseName) {
    try {
        // 1. Ask for professors using the SAVED ID
        std::string target = "/get-professors?Id=" + std::to_string(m_currentCourseId);
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
            Profs[nameStr] = atoi(idStr.c_str());  // Store the mapping of professor name to ID

            // Create Items
            QTableWidgetItem *rank = new QTableWidgetItem(QString::number(row + 1));
            QPushButton* name = new QPushButton(QString::fromStdString(nameStr));
            name->setFlat(true);
            name->setCursor(Qt::PointingHandCursor);
            name->setStyleSheet(
                "QPushButton { color: white; text-decoration: underline; background: transparent; border: none; }"
                "QPushButton:hover { color: rgba(255, 255, 255, 0.6); }"
            );
            QObject::connect(name, &QPushButton::clicked, this, [this, nameStr, CourseName]() {
                professorPage(nameStr, CourseName);
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
            int currentCID = this->m_currentCourseId;
            connect(up, &QPushButton::clicked, this, [this, idStr, currentCID, CourseName]() {
                handleUpvote(idStr, currentCID, CourseName);
            });

            connect(down, &QPushButton::clicked, this, [this, idStr, currentCID, CourseName]() {
                handleDownvote(idStr, currentCID, CourseName);
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
            this->refreshList(CourseName);
        }
    } catch (std::exception& e) {
        std::cout << "Downvote failed: " << e.what() << std::endl;
    }
}


void MainWindow::on_backButton_clicked()
{
    // 1. Go back to the Course Selection page (Index 3 based on your earlier code)
    ui->stackedWidget->setCurrentIndex(3);

            // 2. Shrink the window back to its "Normal" size
    this->showNormal();

            // 3. Optional: Center the widget again if it looks off
    CenterWidget(3, ui->widget_3);
}



void MainWindow::CreateComment(Comment comment)
{
    QWidget* card = new QWidget();
    card->setObjectName("commentCard");
    card->setMinimumHeight(100);
    card->setMinimumWidth(600);
    card->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    card->setStyleSheet(
        "QWidget#commentCard {"
        "   background-color: rgba(255, 255, 255, 0.12);"
        "   border: 1px solid rgba(255, 255, 255, 0.25);"
        "   border-radius: 16px;"
        "   margin: 7px 4px;"
		"   padding: 3px;"
        "}"
    );

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    QVBoxLayout* mainLayout = new QVBoxLayout(card);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(10);

    QLabel* usernameLabel = new QLabel(QString::fromStdString(comment.name));
    usernameLabel->setStyleSheet(
        "font-family: 'Segoe UI';"
        "font-size: 16px;"
        "font-weight: bold;"
        "color: rgba(255, 255, 255, 0.95);"
        "background: transparent;"
        "border: none;"
    );

    QLabel* contentLabel = new QLabel(QString::fromStdString(comment.Content));
    contentLabel->setWordWrap(true);
    contentLabel->setMinimumHeight(40);
    contentLabel->setStyleSheet(
        "font-family: 'Segoe UI';"
        "font-size: 15px;"
        "color: rgba(255, 255, 255, 0.85);"
        "background: transparent;"
        "border: none;"
    );

    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    QLabel* datetimeLabel = new QLabel(QString::fromStdString(comment.PrintTime()));
    datetimeLabel->setStyleSheet(
        "font-family: 'Segoe UI';"
        "font-size: 12px;"
        "color: rgba(255, 255, 255, 0.45);"
        "background: transparent;"
        "border: none;"
    );
    bottomLayout->addWidget(datetimeLabel);

    mainLayout->addWidget(usernameLabel);
    mainLayout->addWidget(contentLabel);
    mainLayout->addLayout(bottomLayout);

	// Add card to the top of the comments section
	QBoxLayout* layout = qobject_cast<QBoxLayout*>(ui->widget_7->layout()); // cast as QBoxLayout to use insertWidget
    if (layout)
        layout->insertWidget(0, card);
}

void MainWindow::DisplayComments() {
	for (Comment comment : Comments) {
        CreateComment(comment);
    }
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

void MainWindow::on_postComment_clicked() {
	std::string ProfName = ui->professorName->text().toStdString().substr(4); // Remove "Dr.  " prefix
    std::string CourseName = ui->courseName->text().toStdString();
    std::string content = ui->commentLineEdit->text().toStdString();
    if (content.empty()) return; // Don't post empty comments
    try {
        // Let's now form the JSON to send the data to the server.
        boost::json::object comment;
		int UserID = user->GetID();

		comment["user_id"] = UserID;
		comment["content"] = content;
        comment["prof_id"] = Profs[ProfName];
        comment["course_id"] = Courses[CourseName];

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
        Comment commentObj(commentID, UserID, user->GetUsername(), content, timestamp);
		Comments.push_back(commentObj); 
        


		// Display the new comment
		CreateComment(commentObj);

    }
    catch (std::exception& e) {
        std::cout << "Connection failed: " << e.what() << std::endl;
    }
	ui->commentLineEdit->clear(); // Clear the input field after posting
}
