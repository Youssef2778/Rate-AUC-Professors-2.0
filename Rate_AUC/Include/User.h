#pragma once
#include <iostream>
#include <string>
using namespace std;

class User {
private:
    string Username;
    string Password;
    string Email;
    const int ID;

    bool verification() {
        //Verify Email
    }

public:

    User(string username, string password,string email, int id) : ID(id) {
		Username = username;
		Password = password;
		Email = email;
	}


    string GetUsername() {
        return Username;
    }

    string GetPassword() {
        return Password;
    }

    string GetEmail() {
        return Email;
    }

    int GetID() {
        return ID;
	}
};
