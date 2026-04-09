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

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

boost::asio::thread_pool pool(std::thread::hardware_concurrency());

void handle_request(const http::request<http::string_body> &req)
{
    std::cout << "Request target: " << req.target() << "\n";

    // The user clicks on the "Register" button on the register page..
    if (req.target() == "/register") {
        auto parsed = json::parse(req.body());
        json::object obj = parsed.as_object();
        try {
            // Let's update the database!
            sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
            sql::Connection *con = driver->connect("tcp://centerbeam.proxy.rlwy.net:11239",
                                                   "root",
                                                   "lTfeKOSlLMYPoYSyCQXLVBXKugsnOAHk");
            con->setSchema("railway");

            sql::Statement *stmt = con->createStatement();
            sql::PreparedStatement *pstmt(
                con->prepareStatement("INSERT INTO users (username, email, encrypted_password, "
                                      "account_type) VALUES (?, ?, ?, ?)"));
            pstmt->setString(1, (std::string) obj["username"].as_string());
            pstmt->setString(2, (std::string) obj["email"].as_string());
            pstmt->setString(3, (std::string) obj["hashed_password"].as_string());
            pstmt->setString(4, "S");

            pstmt->executeUpdate();
        } catch (sql::SQLException &e) {
            std::cerr << "Error: " << e.what() << "(Error code: " << e.getErrorCode() << ")"
                      << std::endl;
        }
    }
}

int main()
{
    net::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
    tcp::endpoint endpoint(tcp::v4(), 8080);
    while (true) {
        tcp::socket socket(io);

        acceptor.accept(socket);

        // A Cout statement for debugging purposes. It ensures that we established a TCP connection with the client
        std::cout << "Client Connected!" << std::endl;
        boost::system::error_code ignored_error;

        // This ensures that we are able to exchange data with the user.
        std::string msg = "Connected!";
        boost::asio::write(socket, boost::asio::buffer(msg), ignored_error);

        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        // We are using a pool of threads for the server to handle requests asynchronously.
        // This means that when we have several users making several requests to the server simultaneously, the server will be able to handle them without much delay.
        boost::asio::post(pool, [req]() { handle_request(req); });

        // yayyyy
    }

    return 0;
}
