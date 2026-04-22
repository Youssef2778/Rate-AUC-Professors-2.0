#pragma once
#include <iostream>
#include <string>
using namespace std;

struct Comment {
    int ID;
	int UserID;
	string name;
    string Content;
    string timestamp;

    string PrintTime() {
		string Date = timestamp.substr(0, 10);
        string Time = timestamp.substr(11, 8);
		return "Posted on " + Date + " at " + Time;
	}
};
