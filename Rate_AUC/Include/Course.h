#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "Teacher.h"
using namespace std;

class Course {
private:
    string Name;
    vector<Teacher> Teachers;
    int ID;

public:
	Course(string name, int id) : ID(id) {
		Name = name;
	}

    vector<Teacher> Leaderboard() {
        bool Sorted;
        for (int i = 0; i < Teachers.size(); i++)
        {
            Sorted = true;
            for (int j = 0; j < Teachers.size() - i - 1; j++)
            {
                if (Teachers[j].GetRating(Name) < Teachers[j + 1].GetRating(Name)) {
                    std::swap(Teachers[j], Teachers[j + 1]);
                    Sorted = false;
                }
            }
            if (Sorted) {
                break;
            }
        }
		return Teachers;
    }

    string GetName() {
		return Name;
    }

    bool AddTeacher(Teacher teacher) {
		for (Teacher T : Teachers) {
            if (T.GetID() == teacher.GetID()) {
                return false;
            }
        }
		Teachers.push_back(teacher);
		return true;
    }
};
