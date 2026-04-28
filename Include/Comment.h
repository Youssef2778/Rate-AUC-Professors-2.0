#pragma once
#include <iostream>
#include <string>
using namespace std;

struct Comment 
{
    int ID;
	int UserID;
	string name;
    string Content;
    string timestamp;
    int flairID;

	Comment(int id, int user_id, string name, string content, string timestamp, int fID)
        : ID(id), UserID(user_id), name(name), Content(content), flairID(fID) {
		this->timestamp = ConvertTimeZone(timestamp);
	}

    // This function converts the timestamp from UTC to Cairo's local time zone (UTC+3) and handles day/month/year overflow(written with the help of claude ai)
    string ConvertTimeZone(const string& timestamp) {
        // The timestamp from the server is in UTC, we need to convert it to local time zone before displaying it.
        // Cairo's timezone is UTC+3, so we will add 2 hours to the timestamp.
        int hrs = stoi(timestamp.substr(11, 2));
        int mins = stoi(timestamp.substr(14, 2));
        int secs = stoi(timestamp.substr(17, 2));
        int day = stoi(timestamp.substr(8, 2));
        int mon = stoi(timestamp.substr(5, 2));
        int year = stoi(timestamp.substr(0, 4));

        hrs += 3; // UTC+3

        // Handle overflow
        if (hrs >= 24) {
            hrs -= 24;
            day++;
            // Days in each month
            int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
            // Leap year check
            if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
                daysInMonth[1] = 29;
            }
            if (day > daysInMonth[mon - 1]) {
                day = 1;
                mon++;
                if (mon > 12) { mon = 1; year++; }
            }
        }

        // Reconstruct the timestamp string
        auto pad = [](int n) { return (n < 10 ? "0" : "") + to_string(n); };
        return to_string(year) + "-" + pad(mon) + "-" + pad(day) + " " +
            pad(hrs) + ":" + pad(mins) + ":" + pad(secs);
    }



    string PrintTime() {
        string Date = timestamp.substr(0, 10);
        int Hrs = stoi(timestamp.substr(11, 2));
        string Mins = timestamp.substr(14, 2);
        string period;

        if (Hrs >= 12) {
            period = " PM";
            if (Hrs > 12) Hrs -= 12;
        } else {
            period = " AM";
            if (Hrs == 0) Hrs = 12; // Midnight
        }

        return "Posted on " + Date + " at " + to_string(Hrs) + ":" + Mins + period;
    }
};
