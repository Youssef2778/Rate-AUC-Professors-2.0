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
		string Time = timestamp.substr(11, 5); // Extracting HH:MM from the timestamp
		int Hrs = stoi(Time.substr(0, 2));
		if (Hrs > 12) {
			Hrs -= 12;
			Time.replace(0, 2, to_string(Hrs));
			Time += " PM";
		} else {
			Time += " AM";
		}
		return "Posted on " + Date + " at " + Time;
	}
};
