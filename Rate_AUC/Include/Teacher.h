#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include "User.h"
#include "Comment.h"
using namespace std;

class Teacher : public User {
private:
    string Name;
    vector<string> Courses;
    map<string, vector<Comment>> Comments;
    map<string, int> Rating;
    map<string, int> RateCount;

public:

    Teacher(string name, string username, string password, string email, int id) : User(username, password, email, id) {
    }

    void ChangeRating(string CourseName,int Value) {
        Rating[CourseName] += Value;
    }

    void ChangeRatingCount(string CourseName, int Value) {
		RateCount[CourseName] += Value;
    }

    void AddComment(string CourseName, Comment comment) {
		Comments[CourseName].push_back(comment);
    }

    int GetRating(string CourseName) {
		return Rating[CourseName];
    }

    int GetRateCount(string CourseName) {
		return RateCount[CourseName];
    }

    string GetName() {
		return Name;
    }

    vector<string> GetCourses(string CourseName) {
        return Courses;
    }

    vector<Comment> GetComments(string CourseName) {
        return Comments[CourseName];
    }


};
