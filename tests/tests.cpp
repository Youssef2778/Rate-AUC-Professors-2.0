#include "helpers.hpp"
#include "../server.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;         // Added for HTTPS
namespace json = boost::json;
using tcp = net::ip::tcp;


class mockMySQLConnectionPool : public MySQLConnectionPool {
   public:
    mockMySQLConnectionPool(int size) : MySQLConnectionPool() {
        for (int i = 0; i < size; i++)
            pool.push(reinterpret_cast<sql::Connection*>(new int(i)));
    }
};

TEST(RegistrationValidation, validateEmail) {
    // Valid emails
    EXPECT_EQ(validEmail("aliabohamar@aucegypt.edu"), true);
    EXPECT_EQ(validEmail("student123@aucegypt.edu"), true);
    EXPECT_EQ(validEmail("first.last@aucegypt.edu"), true);
    EXPECT_EQ(validEmail("user+tag@domain.com"), true);
    EXPECT_EQ(validEmail("user@sub.domain.org"), true);

    // Missing structural parts
    EXPECT_EQ(validEmail("oaidnf"), false);           // no @ or domain
    EXPECT_EQ(validEmail(""), false);                  // empty string
    EXPECT_EQ(validEmail("@aucegypt.edu"), false);     // missing local part
    EXPECT_EQ(validEmail("user@"), false);             // missing domain
    EXPECT_EQ(validEmail("user@domain"), false);       // missing TLD

    // Malformed structure
    EXPECT_EQ(validEmail("user@@domain.com"), false);  // double @
    EXPECT_EQ(validEmail("user @domain.com"), false);  // space in local part
    EXPECT_EQ(validEmail("user@domain..com"), false);  // double dot in domain
    EXPECT_EQ(validEmail("@"), false);                 // only @
}

TEST(RoutingValidation, checkRouting) {
    std::unordered_map<std::string, std::function<void(const http::request<http::string_body>&, tcp::socket&)>> route_function;
    net::io_context io;
    tcp::socket socket(io);
    boost::beast::http::request<boost::beast::http::string_body> request(
        boost::beast::http::verb::post, "/login", 11);
    route_function["/login"]           = [](const http::request<http::string_body>&, tcp::socket&){return true;};
    route_function["/register"]        = [](const http::request<http::string_body>&, tcp::socket&){return true;};
    route_function["/get-departments"] = [](const http::request<http::string_body>&, tcp::socket&){return true;};
    route_function["/upvote"]          = [](const http::request<http::string_body>&, tcp::socket&){return true;};
    route_function["/downvote"]        = [](const http::request<http::string_body>&, tcp::socket&){return true;};
    route_function["/comment"]         = [](const http::request<http::string_body>&, tcp::socket&){return true;};

    
    EXPECT_EQ(handle_request(request, socket, route_function), true);

    boost::beast::http::request<boost::beast::http::string_body> request2(
        boost::beast::http::verb::post, "/register", 11);
    EXPECT_EQ(handle_request(request2, socket, route_function), true);

    boost::beast::http::request<boost::beast::http::string_body> request3(
        boost::beast::http::verb::get, "/get-departments", 11);
    EXPECT_EQ(handle_request(request3, socket, route_function), true);

    boost::beast::http::request<boost::beast::http::string_body> request4(
        boost::beast::http::verb::post, "/upvote", 11);
    EXPECT_EQ(handle_request(request4, socket, route_function), true);

    boost::beast::http::request<boost::beast::http::string_body> request5(
        boost::beast::http::verb::post, "/downvote", 11);
    EXPECT_EQ(handle_request(request5, socket, route_function), true);

    boost::beast::http::request<boost::beast::http::string_body> request6(
        boost::beast::http::verb::post, "/comment", 11);
    EXPECT_EQ(handle_request(request6, socket, route_function), true);

    // Unknown route should not match any handler
    boost::beast::http::request<boost::beast::http::string_body> request7(
        boost::beast::http::verb::get, "/unknown-route", 11);
    EXPECT_EQ(handle_request(request7, socket, route_function), false);
}

TEST(ConnectionPoolTest, getReturnsNonNull) {
    mockMySQLConnectionPool pool(1);
    sql::Connection* con = pool.get();
    EXPECT_NE(con, nullptr);
}

TEST(ConnectionPoolTest, releaseRestoresConnection) {
    mockMySQLConnectionPool pool(1);
    sql::Connection* con = pool.get();
    pool.release(con);
    sql::Connection* con2 = pool.get();
    EXPECT_EQ(con, con2);  // same pointer must come back
}

TEST(ConnectionPoolTest, poolOfSizeThreeReturnsDistinctConnections) {
    mockMySQLConnectionPool pool(3);
    sql::Connection* c1 = pool.get();
    sql::Connection* c2 = pool.get();
    sql::Connection* c3 = pool.get();
    EXPECT_NE(c1, nullptr);
    EXPECT_NE(c2, nullptr);
    EXPECT_NE(c3, nullptr);
    EXPECT_NE(c1, c2);
    EXPECT_NE(c2, c3);
    EXPECT_NE(c1, c3);
}

TEST(ConnectionPoolTest, releaseAndReacquireMultiple) {
    mockMySQLConnectionPool pool(2);
    sql::Connection* c1 = pool.get();
    sql::Connection* c2 = pool.get();
    pool.release(c1);
    pool.release(c2);
    sql::Connection* c3 = pool.get();
    sql::Connection* c4 = pool.get();
    EXPECT_NE(c3, nullptr);
    EXPECT_NE(c4, nullptr);
}

