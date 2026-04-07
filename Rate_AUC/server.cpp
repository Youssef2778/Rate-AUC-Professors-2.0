#include <bits/stdc++.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

int main () { 
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

        auto parsed = json::parse(req.body());
        std::cout << "Here it is: " << req.body() << std::endl;
        json::object obj = parsed.as_object();

        http::response<http::string_body> response(http::status::ok, req.version());
        response.set(http::field::content_type, "text/plain");
        response.body() = req.body();
        response.prepare_payload();

        http::write(socket, response);

    }


    return 0;
}