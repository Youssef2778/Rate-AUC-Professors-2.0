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

// Loads the homepage when called.
void MainWindow::HomePage()
{
    ui->stackedWidget->setCurrentIndex(3);
    CenterWidget(3, ui->widget_3);
    
    ui->DepartmentCB->clear();
    ui->CourseCB->clear();  
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
            Deps[name] = ID;  // Store the mapping of department ID to name
            // populate the QComboBox
            ui->DepartmentCB->addItem(QString::fromStdString(name));
        }
    } catch (std::exception& e) {
        std::cout << "Failed: " << e.what() << std::endl;
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
            Courses[ID] = name;  // Store the mapping of course ID to name
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
    if (CourseName.empty()) return; // Don't proceed if no course is selected

    //find the course ID
    for (const auto& pair : Courses) {
        if (pair.second == CourseName) {
            user->SetCurrentCourseID(pair.first);
            break;
        }
    }

    LeaderboardPage();
}