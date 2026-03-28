#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Course.h"
using namespace std;

class Department {
private:
    string Name;
    vector<Course> Courses;
    int ID;

public:

    Department(string name, int id) : ID(id) {}

    string GetName() {
		return Name;
    }

    int GetID() {
		return ID;
    }

    vector<Course> GetCourses(string CourseName) {
        return Courses;
    }
};
