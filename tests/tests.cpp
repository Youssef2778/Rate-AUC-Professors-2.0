#include "helpers.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

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


