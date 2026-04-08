#pragma once
#include <iostream>
#include <string>
#include "Teacher.h"
using namespace std;

struct Vote {
    string CourseName;
    Teacher* teacher;
    int Value;

    bool operator==(const Vote v) {
        if (this->CourseName == v.CourseName && this->teacher == v.teacher)
        {
            return true;
        }
        return false;
    }

};
