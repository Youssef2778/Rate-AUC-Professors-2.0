#include "helpers.hpp"

// Validates an email address against a strict regex pattern.
// The regex enforces:
//   - Local part: alphanumeric/special chars allowed; dots permitted only between
//     non-dot segments (no consecutive dots, no leading/trailing dots).
//   - Domain labels: each label must start and end with an alphanumeric character
//     (no leading/trailing hyphens, no consecutive dots between labels).
//   - TLD: must be 2 or more alphabetic characters.
// Note: does NOT enforce the AUC domain (@aucegypt.edu) — that check is done
// separately in registerPage.cpp.
bool validEmail(std::string email) {
    std::regex pattern(
        R"(^[a-zA-Z0-9_%+-]+(?:\.[a-zA-Z0-9_%+-]+)*@(?:[a-zA-Z0-9](?:[a-zA-Z0-9-]*[a-zA-Z0-9])?\.)+[a-zA-Z]{2,}$)");
    if (std::regex_match(email, pattern))
        return true;
    return false;
}