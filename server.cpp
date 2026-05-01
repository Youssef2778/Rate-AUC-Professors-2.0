#include "server.hpp"

int main()
{
    std::unordered_map<std::string, std::function<void(const http::request<http::string_body>&, tcp::socket&)>> route_function;
    route_function["/login"] = Login;
    route_function["/register"] = Register;
    route_function["/get-departments"] = GetDepartments;
    route_function["/upvote"] = Upvote;
    route_function["/downvote"] = Downvote;
    route_function["/comment"] = Comment;
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
        boost::asio::post(pool, [socket = std::move(socket), &route_function]() mutable {
            try {
                beast::flat_buffer buffer;
                while (true) {
                    http::request<http::string_body> req;
                    boost::system::error_code ec;
                    http::read(socket, buffer, req, ec);
                    if (ec == http::error::end_of_stream || ec)
                        break;
                    handle_request(req, socket, route_function);
                }
            } catch (std::exception& e) {
                std::cerr << "Thread error: " << e.what() << std::endl;
            }
            std::cout << "Client disconnected." << std::endl;
        });
    }
    return 0;
}
