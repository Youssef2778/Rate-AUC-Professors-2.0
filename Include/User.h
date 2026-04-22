#pragma once
#include <iostream>
#include <string>
using namespace std;

class User
{
   private:
    string Username;
    string Email;
    const int ID;

   public:
    User(string username, string email, int id) : ID(id)
    {
        Username = username;
        Email = email;
        cout << "Session initiated with username: " << Username << ", email: " << Email << ", and ID: " << ID << endl;
    }

    string GetUsername()
    {
        return Username;
    }


    string GetEmail()
    {
        return Email;
    }

    int GetID()
    {
        return ID;
    }
};
