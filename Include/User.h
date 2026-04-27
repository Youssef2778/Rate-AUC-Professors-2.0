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
	int CurrentCourseID; 
	int CurrentProfID;
    bool Connected; 


   public:
    User(string username, string email, int id) : ID(id)
    {
        Username = username;
        Email = email;
		CurrentCourseID = -1; // Initialize with an invalid course ID
        CurrentProfID = -1; // Initialize with an invalid professor ID
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

    int GetCurrentCourseID()
    {
        return CurrentCourseID;
    }

    void SetCurrentCourseID(int courseID)
    {
        CurrentCourseID = courseID;
    }

    int GetCurrentProfID()
    {
        return CurrentProfID;
	}

    void SetCurrentProfID(int profID)
    {
        CurrentProfID = profID;
	}

    bool IsConnected()
    {
        return Connected;
    }

    void SetConnected(bool status)
    {
        Connected = status;
    }
};