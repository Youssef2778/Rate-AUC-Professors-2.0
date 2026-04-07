#include <bits/stdc++.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
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
    }
    return 0;
}