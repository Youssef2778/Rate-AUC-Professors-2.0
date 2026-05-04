// server.hpp — HTTP request handlers and connection pool for the Rate AUC Professor backend.
// All route handler functions follow the signature:
//   void Handler(const http::request<http::string_body>&, tcp::socket&)
// They read from the global dbPool, execute SQL, and write an HTTP response directly to the socket.

#include <iostream>
#include <string>
#include <thread>
#include <exception>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <mutex>
#include <queue>
#include <fstream>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include "bcrypt/BCrypt.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace json = boost::json;
using tcp = net::ip::tcp;

// Thread pool sized to the number of hardware threads so concurrent client requests
// are handled in parallel without spawning unbounded threads.
boost::asio::thread_pool pool(std::thread::hardware_concurrency());

// Manages a fixed set of reusable MySQL connections. Connections are opened once at
// startup (in parallel) and leased/returned via get()/release(). The protected no-arg
// constructor and protected members exist so unit tests can inject fake connections
// without hitting a real database.
class MySQLConnectionPool
{
   protected:
    std::queue<sql::Connection*> pool;
    std::mutex mtx;
    MySQLConnectionPool() {}  // for subclasses that populate pool directly

   public:
    // Opens a single connection to the remote MySQL instance.
    // Virtual so tests can override it via a mock subclass.
    virtual sql::Connection* connect() {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        sql::Connection* con = driver->connect("tcp://centerbeam.proxy.rlwy.net:11239",
                                               "root", "lTfeKOSlLMYPoYSyCQXLVBXKugsnOAHk");
        con->setSchema("railway");
        return con;
    }

    // Opens `size` connections in parallel and stores them in the pool queue.
    MySQLConnectionPool(int size) {
        std::vector<std::thread> threads;
        std::mutex poolMtx;
        for (int i = 0; i < size; i++) {
            threads.emplace_back([this, &poolMtx]() {
                sql::Connection* con = connect();
                std::unique_lock<std::mutex> lock(poolMtx);
                pool.push(con);
            });
        }
        for (auto& t : threads)
            t.join();
    }

    // Removes and returns the next available connection. Blocks until one is free.
    sql::Connection* get()
    {
        std::unique_lock<std::mutex> lock(mtx);
        sql::Connection* con = pool.front();
        pool.pop();
        return con;
    }

    // Returns a connection to the pool after a handler is done using it.
    void release(sql::Connection* con)
    {
        std::unique_lock<std::mutex> lock(mtx);
        pool.push(con);
    }
};

// Global connection pool pointer — initialized in main() before the accept loop.
MySQLConnectionPool* dbPool = nullptr;

// Sends all comments for a professor/course pair to the Groq LLM API and returns
// a short AI-generated summary with an overall vibe, strengths, and weaknesses.
// Uses curl via popen because the server has no async HTTP client dependency.
std::string generateSummaryFromHuggingFace(const std::string& allComments) {
    try {
        std::string apiKey = "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";

        // 1. Prepare JSON (Groq uses the 'messages' format which needs to be an array)
        boost::json::object userMessage;
        userMessage["role"] = "user";
        userMessage["content"] = "Summarize these professor reviews into a short 'Overall Vibe' and bullet points for Strengths and Weaknesses:\n\n" + allComments;

        boost::json::array messages;
        messages.push_back(userMessage);

        //sending the data to groq
        boost::json::object payload;
        payload["model"] = "llama-3.3-70b-versatile"; // A powerful, fast model
        payload["messages"] = messages;

        // serlise json so it can be sent in the http request
        std::string json_body = boost::json::serialize(payload);

        // 2. Save to file
        std::ofstream temp_file("ai_payload.json");
        temp_file << json_body;
        temp_file.close();

        // 3. Construct the curl command for Groq
        std::string cmd = "curl -s -X POST 'https://api.groq.com/openai/v1/chat/completions' "
                          "-H 'Authorization: Bearer " + apiKey + "' "
                                     "-H 'Content-Type: application/json' "
                                     "-d @ai_payload.json";

        // 4. Run it
        std::string result;
        FILE* fp = popen(cmd.c_str(), "r");
        if (!fp) return "System error: could not run curl.";

        char buffer[256];
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            result += buffer;
        }
        pclose(fp);
        std::remove("ai_payload.json");

        // 5. Parse Groq's Response
        auto parsed = boost::json::parse(result);
        if (parsed.is_object() && parsed.as_object().contains("choices")) {
            auto choices = parsed.as_object().at("choices").as_array();
            return boost::json::value_to<std::string>(choices[0].as_object().at("message").as_object().at("content"));
        }

        return "AI Error: " + result; // Shows raw error if it fails

    } catch (std::exception const& e) {
        return "Exception: " + std::string(e.what());
    }
}

// Inserts a new user row and returns the auto-generated user ID so the client
// can start a session immediately without a follow-up login round-trip.
void Register(const http::request<http::string_body>& req, tcp::socket& socket){
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

// Returns all department names and IDs sorted alphabetically.
void GetDepartments(const http::request<http::string_body>& req, tcp::socket& socket){
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

// Returns professors ranked by their total vote score for a given course.
// Course ID is passed as a query parameter: /get-leaderboard?CourseId=<id>
void GetLeaderboard(const http::request<http::string_body>& req, tcp::socket& socket){
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
            con->prepareStatement("SELECT professor_id, score FROM course_professor WHERE course_id = ? ORDER BY score DESC"));

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

// Returns all courses belonging to a department, sorted alphabetically.
// Department ID is passed as a query parameter: /get-courses?Id=<id>
void GetCourses(const http::request<http::string_body>& req, tcp::socket& socket){
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

// Looks up the user by email and validates the BCrypt password hash.
// Returns {status: true, id, username} on success, or {status: false, error} on failure.
void Login(const http::request<http::string_body>& req, tcp::socket& socket){
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

// Records a +1 vote for a professor in a course. Uses an upsert so a user can change
// a prior downvote to an upvote but cannot cast two upvotes.
void Upvote(const http::request<http::string_body>& req, tcp::socket& socket){
    std::cout << "\n--- STARTING UPVOTE ROUTE ---" << std::endl;
    sql::Connection* con = nullptr;

    try {
        auto parsed = boost::json::parse(req.body());
        boost::json::object json_obj = parsed.as_object();

                // Extract IDs safely
        int prof_id = json_obj.at("professor_id").is_string() ?
                            std::stoi(json_obj.at("professor_id").as_string().c_str()) : json_obj.at("professor_id").as_int64();

        int course_id = json_obj.at("course_id").is_string() ?
                            std::stoi(json_obj.at("course_id").as_string().c_str()) : json_obj.at("course_id").as_int64();

                // Extract the user_id (Make sure your Qt app is sending this in the JSON!)
        int user_id = json_obj.at("user_id").as_int64();

        con = dbPool->get();

        // THE MAGIC UPSERT QUERY: Insert a +1 vote, or update existing vote to +1
        sql::PreparedStatement* pstmt = con->prepareStatement(
            "INSERT INTO votes (user_id, course_id, professor_id, vote) "
            "VALUES (?, ?, ?, 1) "
            "ON DUPLICATE KEY UPDATE vote = 1"
            );
        pstmt->setInt(1, user_id);
        pstmt->setInt(2, course_id);
        pstmt->setInt(3, prof_id);

        int rows = pstmt->executeUpdate();
        delete pstmt;
        dbPool->release(con);

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.prepare_payload();
        http::write(socket, res);

    } catch (sql::SQLException& e) {
        std::cout << "\n[SQL Error] " << e.what() << std::endl;
        if (con) dbPool->release(con);
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.prepare_payload();
        http::write(socket, res);
    } catch (std::exception& e) {
        std::cout << "\n[JSON Error] " << e.what() << std::endl;
        if (con) dbPool->release(con);
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.prepare_payload();
        http::write(socket, res);
    }
}

// Records a -1 vote for a professor in a course. Mirror of Upvote — upsert enforces
// one vote per (user, course, professor) tuple.
void Downvote(const http::request<http::string_body>& req, tcp::socket& socket){
    std::cout << "\n--- STARTING DOWNVOTE ROUTE ---" << std::endl;
    sql::Connection* con = nullptr;

    try {
        auto parsed = boost::json::parse(req.body());
        boost::json::object json_obj = parsed.as_object();

        int prof_id = json_obj.at("professor_id").is_string() ?
                            std::stoi(json_obj.at("professor_id").as_string().c_str()) : json_obj.at("professor_id").as_int64();

        int course_id = json_obj.at("course_id").is_string() ?
                            std::stoi(json_obj.at("course_id").as_string().c_str()) : json_obj.at("course_id").as_int64();

        int user_id = json_obj.at("user_id").as_int64();

        con = dbPool->get();

        // THE MAGIC UPSERT QUERY: Insert a -1 vote, or update existing vote to -1
        sql::PreparedStatement* pstmt = con->prepareStatement(
            "INSERT INTO votes (user_id, course_id, professor_id, vote) "
            "VALUES (?, ?, ?, -1) "
            "ON DUPLICATE KEY UPDATE vote = -1"
            );
        pstmt->setInt(1, user_id);
        pstmt->setInt(2, course_id);
        pstmt->setInt(3, prof_id);

        int rows = pstmt->executeUpdate();
        delete pstmt;
        dbPool->release(con);

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.prepare_payload();
        http::write(socket, res);

    } catch (sql::SQLException& e) {
        std::cout << "\n[SQL Error] " << e.what() << std::endl;
        if (con) dbPool->release(con);
        // Error responses omitted for brevity, identical to upvote...
    } catch (std::exception& e) {
        std::cout << "\n[JSON Error] " << e.what() << std::endl;
        if (con) dbPool->release(con);
    }
}

// Returns all professors teaching a course, with each professor's live vote score
// computed by summing the votes table. Course ID: /get-professors?Id=<id>
void GetProfessors(const http::request<http::string_body>& req, tcp::socket& socket){
    boost::json::array professors;
    std::string target = std::string(req.target());
    std::string CourseID;
    // Extract the course ID from the query parameter
    size_t pos = target.find("?Id=");
    if (pos != std::string::npos)
        CourseID = target.substr(pos + 4); // 4 = length of "?Id="
    try {
        sql::Connection* con = dbPool->get();
        // Dynamically calculates the score by summing up the votes table!
        sql::PreparedStatement* pstmt(
            con->prepareStatement(
                "SELECT pc.professor_id, p.name, COALESCE(SUM(v.vote), 0) AS score "
                "FROM course_professor pc "
                "JOIN professors p ON pc.professor_id = p.id "
                "LEFT JOIN votes v ON v.professor_id = pc.professor_id AND v.course_id = pc.course_id "
                "WHERE pc.course_id = ? "
                "GROUP BY pc.professor_id, p.name "
                "ORDER BY score DESC;"
                )
            );

        pstmt->setString(1, CourseID);
        sql::ResultSet* res = pstmt->executeQuery();

        while (res->next()) {
            boost::json::object row;
            row["id"] = (int)res->getInt("professor_id");
            row["name"] = (std::string)res->getString("name");
            row["score"] = std::to_string(res->getInt("score"));
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

// Returns all comments for a professor/course pair in chronological order.
// Query parameters: /get-comments?CourseId=<id>&ProfId=<id>
void GetComments(const http::request<http::string_body>& req, tcp::socket& socket){
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
            con->prepareStatement("SELECT usr.username, cmnt.user_id, cmnt.id, cmnt.content, cmnt.timestamp, cmnt.flair_ids FROM comments cmnt JOIN users usr ON cmnt.user_id = usr.id WHERE cmnt.course_id = ? AND cmnt.professor_id = ? "
                "ORDER BY cmnt.timestamp ASC"));
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
            row["flairs"] = (std::string)res->getString("flair_ids");
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

// Inserts a new comment and returns the generated ID and server timestamp.
// Flairs are stored as a JSON array serialised to a VARCHAR column.
void Comment(const http::request<http::string_body>& req, tcp::socket& socket){
    auto parsed = json::parse(req.body());
    json::object comment = parsed.as_object();
    boost::json::object Response;
    try {
        sql::Connection* con = dbPool->get();
        sql::PreparedStatement* pstmt(
            con->prepareStatement("INSERT INTO comments (professor_id, course_id, user_id, content, flair_ids) VALUES (?, ?, ?, ?, ?)"));
        pstmt->setInt(1, comment["prof_id"].as_int64());
        pstmt->setInt(2, comment["course_id"].as_int64());
        pstmt->setInt(3, comment["user_id"].as_int64());
        pstmt->setString(4, (std::string)comment["content"].as_string());
        
        // flairs should be a JSON array in the request body
        auto& flairs = comment["flairs"].as_array();
        pstmt->setString(5, boost::json::serialize(flairs));
        
        pstmt->executeUpdate();
        
        // Retrieve the generated comment ID and timestamp
        sql::Statement* stmt = con->createStatement();
        sql::ResultSet* Res = stmt->executeQuery("SELECT LAST_INSERT_ID(), NOW()");
        while (Res->next()) {
            Response["id"] = (int)Res->getInt(1);
            Response["timestamp"] = (std::string)Res->getString(2);
        }

        delete stmt;
        delete Res;
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
    res.body() = boost::json::serialize(Response);
    res.prepare_payload();
    http::write(socket, res);
}

// Fetches all comment text for a professor/course, then calls the Groq LLM to produce
// an AI summary. Returns a placeholder string when there are no comments yet.
void GetSummary(const http::request<http::string_body>& req, tcp::socket& socket){
    boost::json::object summaryResponse;
        std::string target = std::string(req.target());
        std::string Course_ID;
        std::string Prof_ID;

        // Parse query params exactly like in /get-comments
        size_t pos = target.find("?CourseId=");
        if (pos != std::string::npos)
            Course_ID = target.substr(pos + 10);
        pos = Course_ID.find("&ProfId=");
        if (pos != std::string::npos) {
            Prof_ID = Course_ID.substr(pos + 8);
            Course_ID = Course_ID.substr(0, pos);
        }

        std::string allComments = "";
        try {
            sql::Connection* con = dbPool->get();
            sql::PreparedStatement* pstmt(
                con->prepareStatement("SELECT content FROM comments WHERE course_id = ? AND professor_id = ?")
                );
            pstmt->setString(1, Course_ID);
            pstmt->setString(2, Prof_ID);
            sql::ResultSet* res = pstmt->executeQuery();

            // Build one giant string of all comments
            while (res->next()) {
                allComments += res->getString("content").asStdString() + "\n";
            }
            delete res;
            delete pstmt;
            dbPool->release(con);

            if (allComments.empty()) {
                summaryResponse["summary"] = "Not enough data yet. Be the first to leave a review!";
            } else {
                // Call the Hugging Face AI helper function
                std::string aiSummary = generateSummaryFromHuggingFace(allComments);
                summaryResponse["summary"] = aiSummary;
            }
        }
        catch (sql::SQLException& e) {
            std::cerr << "Database Error generating summary: " << e.what() << std::endl;
            summaryResponse["summary"] = "Could not fetch data for summary.";
        }

        // Send the AI summary back to the Qt client
        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::content_type, "application/json");
        res.body() = boost::json::serialize(summaryResponse);
        res.prepare_payload();
        http::write(socket, res);
}

// Dispatches an incoming HTTP request to the appropriate handler.
// Exact-match routes (login, register, upvote, downvote, comment) are looked up in
// route_function. Prefix-match routes (get-*) are handled with starts_with checks
// because they carry query parameters that prevent an exact key match.
// Returns true if a handler was found, false for unknown routes.
bool handle_request(const http::request<http::string_body>& req,
                    tcp::socket& socket,
                    std::unordered_map<std::string, std::function<void(const http::request<http::string_body>&, tcp::socket&)>> route_function)
{
    std::cout << "Request target: " << req.target() << "\n";
    auto it = route_function.find(req.target());
    if (it != route_function.end()) { it->second(req, socket); return true; }
    else if (req.target().starts_with("/get-leaderboard")) {GetLeaderboard(req, socket); return true;}
    else if (req.target().starts_with("/get-courses")) {GetCourses(req, socket); return true;}
    else if (req.target().starts_with("/get-professors")) {GetProfessors(req, socket); return true;}
    else if (req.target().starts_with("/get-comments")) {GetComments(req, socket); return true;}
    else if (req.target().starts_with("/get-summary")) {GetSummary(req, socket); return true;}

    std::cout << "Unidentified Request: " << req.target() << std::endl;
    return false;
}
