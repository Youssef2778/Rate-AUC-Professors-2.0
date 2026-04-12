#include <bits/stdc++.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <queue>
#include <mutex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

boost::asio::thread_pool pool(std::thread::hardware_concurrency());

//This class was generated with the help of Claude, to handle multiple requests simultaneously instead of establishing a new connection for each request. 
class MySQLConnectionPool {
    std::queue<sql::Connection*> pool;
    std::mutex mtx;
public:
	//Establishing the connections at startup and storing them in the pool
    MySQLConnectionPool(int size) {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        for (int i = 0; i < size; i++) {
            sql::Connection* con = driver->connect("tcp://centerbeam.proxy.rlwy.net:11239",
                "root", "lTfeKOSlLMYPoYSyCQXLVBXKugsnOAHk");
            con->setSchema("railway");
            pool.push(con);
        }
    }

    sql::Connection* get() {
        std::unique_lock<std::mutex> lock(mtx);
        sql::Connection* con = pool.front();
        pool.pop();
        return con;
    }

    void release(sql::Connection* con) {
        std::unique_lock<std::mutex> lock(mtx); 
        pool.push(con);
    }
};

MySQLConnectionPool* dbPool = nullptr; // Global pointer to the connection pool

void handle_request(const http::request<http::string_body>& req, tcp::socket& socket) //Socket passed to write back response
{
    std::cout << "Request target: " << req.target() << "\n";

    // The user clicks on the "Register" button on the register page..
    if (req.target() == "/register") {
        auto parsed = json::parse(req.body());
        json::object obj = parsed.as_object();
        try {
            // Let's update the database!
			sql::Connection* con = dbPool->get(); // Get a connection from the pool

            sql::PreparedStatement* pstmt(
                con->prepareStatement("INSERT INTO users (username, email, encrypted_password, "
                    "account_type) VALUES (?, ?, ?, ?)"));
            pstmt->setString(1, (std::string)obj["username"].as_string());
            pstmt->setString(2, (std::string)obj["email"].as_string());
            pstmt->setString(3, (std::string)obj["hashed_password"].as_string());
            pstmt->setString(4, "S");

            pstmt->executeUpdate();
            delete pstmt;
			dbPool->release(con); // Release the connection back to the pool
        }
        catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << "(Error code: " << e.getErrorCode() << ")"
                << std::endl;
        }
    }
    else if (req.target() == "/get-departments") {
        boost::json::array departments;
        try {
            sql::Connection* con = dbPool->get();
            sql::PreparedStatement* pstmt(
                con->prepareStatement("SELECT department_name, id FROM departments ORDER BY department_name ASC"));
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

        // Send the response back
        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::content_type, "application/json");
        res.body() = boost::json::serialize(departments);
        res.prepare_payload();
        http::write(socket, res);
    }
    else if (req.target().starts_with("/get-courses")) {
        boost::json::array courses;

        std::string target = std::string(req.target());
        std::string DeptID;
        //Extract the department ID from the query parameter
        size_t pos = target.find("?Id=");
        if (pos != std::string::npos)
            DeptID = target.substr(pos + 4); // 4 = length of "?Id="
        try {
            sql::Connection* con = dbPool->get();
            sql::PreparedStatement* pstmt(
                con->prepareStatement("SELECT course_name, id FROM courses WHERE Department_id = ? ORDER BY course_name ASC"));
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
        }
        catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << " (Error code: " << e.getErrorCode() << ")" << std::endl;
        }

        // Send the response back
        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::content_type, "application/json");
        res.body() = boost::json::serialize(courses);
        res.prepare_payload();
        http::write(socket, res);
    }
}

int main()
{
    std::cout << "Server starting..." << std::endl;

    try {
        std::cout << "Connecting to DB pool..." << std::endl;
		dbPool = new MySQLConnectionPool(5); // Initialize the connection pool with 5 connections
        std::cout << "DB pool ready." << std::endl;
    }
    catch (sql::SQLException& e) {
        std::cerr << "Failed to initialize DB pool: " << e.what() << std::endl;
        return 1;
    }
    catch (std::exception& e) {
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

        // A Cout statement for debugging purposes. It ensures that we established a TCP connection with the client
        std::cout << "Client Connected!" << std::endl;
        boost::system::error_code ignored_error;

        // This ensures that we are able to exchange data with the user.
        std::string msg = "Connected!";
        boost::asio::write(socket, boost::asio::buffer(msg), ignored_error);

        // We are using a pool of threads for the server to handle requests asynchronously.
        // This means that when we have several users making several requests to the server simultaneously, the server will be able to handle them without much delay.
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
            }
            catch (std::exception& e) {
                std::cerr << "Thread error: " << e.what() << std::endl;
            }
            // yayyyy
            std::cout << "Client disconnected." << std::endl;
        });
    }
    return 0;
}