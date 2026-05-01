#include "helpers.hpp"

bool validEmail(std::string email) {
    std::regex pattern(R"(^[a-zA-Z0-9_%+-]+(?:\.[a-zA-Z0-9_%+-]+)*@(?:[a-zA-Z0-9](?:[a-zA-Z0-9-]*[a-zA-Z0-9])?\.)+[a-zA-Z]{2,}$)");
    if (std::regex_match(email, pattern)) return true;
    return false;
}