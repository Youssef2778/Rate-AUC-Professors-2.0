// server.cpp — Entry point for the Rate AUC Professor HTTP server.
// Registers all route handlers, initialises the MySQL connection pool, then enters
// an accept loop that dispatches each client connection to the thread pool.

#include "server.hpp"

int main()
{
    // Exact-match routes are stored in a map so handle_request can dispatch in O(1).
    std::unordered_map<std::string, std::function<void(const http::request<http::string_body>&, tcp::socket&)>> route_function;
    route_function["/login"] = Login;
    route_function["/register"] = Register;
    route_function["/get-departments"] = GetDepartments;
    route_function["/upvote"] = Upvote;
    route_function["/downvote"] = Downvote;
    route_function["/comment"] = Comment;
    std::cout << "Server starting..." << std::endl;

    // MySQL Connector/C++ has a one-time internal setup triggered by the first call to
    // get_mysql_driver_instance(). Calling it from a worker thread while other threads
    // are also calling it causes a crash. Pre-warming it here on the main thread before
    // any threads are spawned avoids that race.
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        (void)driver;
    } catch (...) {
        // Ignore — failure here is non-fatal; pool construction will surface real errors.
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
        std::cout << "Client Connected!" << std::endl;

        // Send a plain-text greeting so the client knows the TCP handshake succeeded
        // before it sends its first HTTP request.
        boost::system::error_code ignored_error;
        std::string msg = "Connected!";
        boost::asio::write(socket, boost::asio::buffer(msg), ignored_error);

        // Hand the socket off to the thread pool so this loop can immediately accept
        // the next client without blocking. The lambda owns the socket via move so
        // it stays alive for the full duration of the session.
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
