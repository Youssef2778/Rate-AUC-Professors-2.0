#include <iostream>
#include <string>
#include <thread>
#include <exception>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_driver.h>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <mutex>
#include <queue>

#include "bcrypt/BCrypt.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

boost::asio::thread_pool pool(std::thread::hardware_concurrency());

// This class was generated with the help of Claude, to handle multiple requests simultaneously
// instead of establishing a new connection for each request.
class MySQLConnectionPool
{
    std::queue<sql::Connection*> pool;
    std::mutex mtx;

   public:
    // Establishing the connections at startup and storing them in the pool
    MySQLConnectionPool(int size) {
        std::vector<std::thread> threads;
        std::mutex poolMtx;
        for (int i = 0; i < size; i++) {
            threads.emplace_back([this, &poolMtx]() {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                sql::Connection* con = driver->connect("tcp://centerbeam.proxy.rlwy.net:11239",
                                                       "root", "lTfeKOSlLMYPoYSyCQXLVBXKugsnOAHk");
                con->setSchema("railway");
                std::unique_lock<std::mutex> lock(poolMtx);
                pool.push(con);
            });
        }
        for (auto& t : threads)
            t.join();
    }

    sql::Connection* get()
    {
        std::unique_lock<std::mutex> lock(mtx);
        sql::Connection* con = pool.front();
        pool.pop();
        return con;
    }

    void release(sql::Connection* con)
    {
        std::unique_lock<std::mutex> lock(mtx);
        pool.push(con);
    }
};

MySQLConnectionPool* dbPool = nullptr;  // Global pointer to the connection pool

void handle_request(const http::request<http::string_body>& req,
                    tcp::socket& socket)  // Socket passed to write back response
{
    std::cout << "Request target: " << req.target() << "\n";

    // The user clicks on the "Register" button on the register page..
    if (req.target() == "/register") {
        auto parsed = json::parse(req.body());
        json::object obj = parsed.as_object();
        try {
            // Let's update the database!
            sql::Connection* con = dbPool->get();  // Get a connection from the pool

            sql::PreparedStatement* pstmt(
                con->prepareStatement("INSERT INTO users (username, email, encrypted_password, "
                                      "account_type) VALUES (?, ?, ?, ?)"));
            pstmt->setString(1, (std::string)obj["username"].as_string());
            pstmt->setString(2, (std::string)obj["email"].as_string());
            pstmt->setString(3, (std::string)obj["hashed_password"].as_string());
            pstmt->setString(4, "S");

            pstmt->executeUpdate();

            // Retrieve the generated user ID
            sql::Statement* stmt = con->createStatement();
            sql::ResultSet* Res = stmt->executeQuery("SELECT LAST_INSERT_ID()");

            int id = -1;
            if (Res->next()) {
                id = Res->getInt(1);
            }

            // Prepare the JSON response with the generated user ID
            json::object Response;
            Response["id"] = id;

            delete Res;
            delete pstmt;
            dbPool->release(con);  // Release the connection back to the pool

            // Send the id back to the client
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = boost::json::serialize(Response);
            res.prepare_payload();
            http::write(socket, res);
        } catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << "(Error code: " << e.getErrorCode() << ")"
                      << std::endl;
        }





    }
    else if (req.target() == "/get-departments") {
        boost::json::array departments;
        try {
            sql::Connection* con = dbPool->get();
            sql::PreparedStatement* pstmt(con->prepareStatement(
                "SELECT department_name, id FROM departments ORDER BY department_name ASC"));
            sql::ResultSet* res = pstmt->executeQuery();
            while (res->next()) {
                boost::json::object row;
                row["department_name"] = (std::string)res->getString("department_name");
                row["id"] = res->getInt("id");
                departments.push_back(row);
            }
            delete res;
            delete pstmt;
            dbPool->release(con);
            }
            catch (sql::SQLException& e) {
                    std::cerr << "Error: " << e.what() << " (Error code: " << e.getErrorCode() << ")" << std::endl;
            }

            http::response<http::string_body> res{ http::status::ok, req.version() };
            res.set(http::field::content_type, "application/json");
            res.body() = boost::json::serialize(departments);
            res.prepare_payload();
            http::write(socket, res);
        }
    else if (req.target().starts_with("/get-leaderboard")) {
            boost::json::array leaderboard;

            std::string target = std::string(req.target());
            std::string CourseID;
            // Extract the course ID from the query parameter
            size_t pos = target.find("?CourseId=");
            if (pos != std::string::npos)
                CourseID = target.substr(pos + 10);

            try {
                sql::Connection* con = dbPool->get();

                // Updated to match your exact table and column names!
                sql::PreparedStatement* pstmt(
                    con->prepareStatement("SELECT professor_id, score FROM professor_course WHERE course_id = ? ORDER BY score DESC"));

                pstmt->setString(1, CourseID);
                sql::ResultSet* res = pstmt->executeQuery();

                while (res->next()) {
                    boost::json::object row;

                    // We convert the professor_id integer into a string so the frontend can display it in the "Name" column for now.
                    row["prof_name"] = std::to_string(res->getInt("professor_id"));
                    row["score"] = res->getInt("score");

                    leaderboard.push_back(row);
                }
                delete res;
                delete pstmt;
                dbPool->release(con);
            }
            catch (sql::SQLException& e) {
                std::cerr << "Error: " << e.what() << " (Error code: " << e.getErrorCode() << ")" << std::endl;
            }

            // Send the response back to the client
            http::response<http::string_body> res{ http::status::ok, req.version() };
            res.set(http::field::content_type, "application/json");
            res.body() = boost::json::serialize(leaderboard);
            res.prepare_payload();
            http::write(socket, res);
    }
    else if (req.target().starts_with("/get-courses")) {
        boost::json::array courses;

        std::string target = std::string(req.target());
        std::string DeptID;
        // Extract the department ID from the query parameter
        size_t pos = target.find("?Id=");
        if (pos != std::string::npos)
            DeptID = target.substr(pos + 4);  // 4 = length of "?Id="
        try {
            sql::Connection* con = dbPool->get();
            sql::PreparedStatement* pstmt(
                con->prepareStatement("SELECT course_name, id FROM courses WHERE Department_id = ? "
                                      "ORDER BY course_name ASC"));
            pstmt->setString(1, DeptID);
            sql::ResultSet* res = pstmt->executeQuery();
            while (res->next()) {
                boost::json::object row;
                row["course_name"] = (std::string)res->getString("course_name");
                row["id"] = res->getInt("id");
                courses.push_back(row);
            }
            delete res;
            delete pstmt;
            dbPool->release(con);
        } catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << " (Error code: " << e.getErrorCode() << ")"
                      << std::endl;
        }

        // Send the response back
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.body() = boost::json::serialize(courses);
        res.prepare_payload();
        http::write(socket, res);
    }
    else if (req.target() == "/login") {
        auto parsed = json::parse(req.body());
        json::object login = parsed.as_object();
        json::object response;
        response["status"] = true;
        try {
            sql::Connection* con = dbPool->get();
            sql::PreparedStatement* pstmt(
                con->prepareStatement("SELECT id, username, encrypted_password FROM users WHERE email = ?"));
            pstmt->setString(1, (std::string)login["email"].as_string());
            sql::ResultSet* res = pstmt->executeQuery();
            if (!res->next()) {
                response["status"] = false;
                response["error"] = "email not found";
            } else {
                response["id"] = res->getInt("id");
                response["username"] = (std::string)res->getString("username");
                if (!BCrypt::validatePassword((std::string)login["password"].as_string(),
                                              res->getString("encrypted_password"))) {
                    response["status"] = false;
                    response["error"] = "incorrect password";
                }
            }

            // Send the response back
            http::response<http::string_body> http_response(http::status::ok, req.version());
            http_response.set(http::field::content_type, "application/json");
            http_response.body() = boost::json::serialize(response);
            http_response.prepare_payload();
            http::write(socket, http_response);
            delete res;
            delete pstmt;
            dbPool->release(con);

            // In this case, we are sending a "report" to the user. The GUI will vary according to
            // the report's contents.

        } catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << " (Error code: " << e.getErrorCode() << ")"
                      << std::endl;
        }
    }
    else if (req.target().starts_with("/get-professors")) {
        boost::json::array professors;
        std::cout << "Handling" << std::endl;
        std::string target = std::string(req.target());
        std::string CourseID;
        // Extract the course ID from the query parameter
        size_t pos = target.find("?Id=");
        if (pos != std::string::npos)
            CourseID = target.substr(pos + 4); // 4 = length of "?Id="
        try {
            std::cout << "before get" << std::endl;
            sql::Connection* con = dbPool->get();
            std::cout << "after get" << std::endl;            sql::PreparedStatement* pstmt(
                con->prepareStatement("SELECT pc.professor_id, p.name, pc.score FROM course_professor pc JOIN professors p ON pc.professor_id = p.id WHERE pc.course_id = ? ORDER BY pc.score DESC;"));
            std::cout << "SQL" << std::endl;

            pstmt->setString(1, CourseID);
            sql::ResultSet* res = pstmt->executeQuery();
            std::cout << "EXEC" << std::endl;

            while (res->next()) {
                boost::json::object row;
                row["id"] = (int)res->getInt("professor_id");
                row["name"] = (std::string)res->getString("name");
                row["score"] = std::to_string(res->getInt("score"));
                std::cout << row["id"] << row["score"] << row["name"] << std::endl;
                professors.push_back(row);
            }
            delete res;
            delete pstmt;
            dbPool->release(con);
        }
        catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << " (Error code: " << e.getErrorCode() << ")" << std::endl;
        }

        // Send the response back
        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::content_type, "application/json");
        res.body() = boost::json::serialize(professors);
        res.prepare_payload();
        http::write(socket, res);
    }
    else if (req.target().starts_with("/get-comments")) {
        boost::json::array Comments;
        std::string target = std::string(req.target());
        std::string Course_ID;
        std::string Prof_ID;
        // Extract the course ID and professor ID from the query parameters
        size_t pos = target.find("?CourseId=");
        if (pos != std::string::npos)
            Course_ID = target.substr(pos + 10);  // 10 = length of "?CourseId="
        pos = Course_ID.find("&ProfId=");
        if (pos != std::string::npos) {
            Prof_ID = Course_ID.substr(pos + 8);  // 8 = length of "&ProfId="
            Course_ID = Course_ID.substr(0, pos);
        }
        try {
            sql::Connection* con = dbPool->get();
            sql::PreparedStatement* pstmt(
                con->prepareStatement("SELECT usr.username, cmnt.user_id, cmnt.id, cmnt.content, cmnt.timestamp FROM comments cmnt JOIN users usr ON cmnt.user_id = usr.id WHERE cmnt.course_id = ? AND cmnt.professor_id = ? "
                    "ORDER BY cmnt.timestamp DESC"));
            pstmt->setString(1, Course_ID);
            pstmt->setString(2, Prof_ID);
            sql::ResultSet* res = pstmt->executeQuery();
            while (res->next()) {
                boost::json::object row;
                row["username"] = (std::string)res->getString("username");
                row["user_id"] = (int)res->getInt("user_id");
                row["id"] = (int)res->getInt("id");
                row["content"] = (std::string)res->getString("content");
                row["timestamp"] = (std::string)res->getString("timestamp");
                Comments.push_back(row);
            }
            delete res;
            delete pstmt;
            dbPool->release(con);
        }
        catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << " (Error code: " << e.getErrorCode() << ")"
                << std::endl;
        }

        // Send the response back
        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::content_type, "application/json");
        res.body() = boost::json::serialize(Comments);
        res.prepare_payload();
        http::write(socket, res);
    }
}

int main()
{
    std::cout << "Server starting..." << std::endl;

    // The MySQL connector library has a one-time internal setup that happens the  first time get_mysql_driver_instance() is called and a call using threading caused conflict and crash
    // Alowing MySQL connector library to initialize propery
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        (void)driver; // function cast as void just for initializing, not using
    } catch (...) {
        // Ignore
    }

    try {
        std::cout << "Connecting to DB pool..." << std::endl;
        dbPool = new MySQLConnectionPool(5);  // Initialize the connection pool with 5 connections
        std::cout << "DB pool ready." << std::endl;
    } catch (sql::SQLException& e) {
        std::cerr << "Failed to initialize DB pool: " << e.what() << std::endl;
        return 1;
    } catch (std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Starting listener..." << std::endl;
    net::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
    tcp::endpoint endpoint(tcp::v4(), 8080);

    std::cout << "Waiting for clients..." << std::endl;
    while (true) {
        tcp::socket socket(io);

        acceptor.accept(socket);

        // A Cout statement for debugging purposes. It ensures that we established a TCP connection
        // with the client
        std::cout << "Client Connected!" << std::endl;
        boost::system::error_code ignored_error;

        // This ensures that we are able to exchange data with the user.
        std::string msg = "Connected!";
        boost::asio::write(socket, boost::asio::buffer(msg), ignored_error);

        // We are using a pool of threads for the server to handle requests asynchronously.
        // This means that when we have several users making several requests to the server
        // simultaneously, the server will be able to handle them without much delay.
        boost::asio::post(pool, [socket = std::move(socket)]() mutable {
            try {
                beast::flat_buffer buffer;
                while (true) {
                    http::request<http::string_body> req;
                    boost::system::error_code ec;
                    http::read(socket, buffer, req, ec);
                    if (ec == http::error::end_of_stream || ec)
                        break;
                    handle_request(req, socket);
                }
            } catch (std::exception& e) {
                std::cerr << "Thread error: " << e.what() << std::endl;
            }
            // yayyyy
            std::cout << "Client disconnected." << std::endl;
        });
    }
    return 0;
}
