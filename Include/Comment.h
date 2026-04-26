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

        Comment(int id, int user_id, string name, string content, string timestamp)
        : ID(id), UserID(user_id), name(name), Content(content){
                this->timestamp = ConvertTimeZone(timestamp);
        }

    // This function converts the timestamp from UTC to Cairo's local time zone (UTC+2) and handles day/month/year overflow(written with the help of claude ai)
    string ConvertTimeZone(const string& timestamp) {
        // The timestamp from the server is in UTC, we need to convert it to local time zone before displaying it.
        // Cairo's timezone is UTC+2, so we will add 2 hours to the timestamp.
        int hrs = stoi(timestamp.substr(11, 2));
        int mins = stoi(timestamp.substr(14, 2));
        int secs = stoi(timestamp.substr(17, 2));
        int day = stoi(timestamp.substr(8, 2));
        int mon = stoi(timestamp.substr(5, 2));
        int year = stoi(timestamp.substr(0, 4));

        hrs += 2; // UTC+2

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
        string Time = timestamp.substr(11, 5); // Extracting HH:MM from the timestamp
        int Hrs = stoi(Time.substr(0, 2));
        if (Hrs >= 12) {
            if (Hrs > 12) {
                Hrs -= 12;
                // Pad single digit hours
                string HrsStr = (Hrs < 10 ? "0" : "") + to_string(Hrs);
                Time.replace(0, 2, HrsStr);
            }
            Time += " PM";
        }
        else {
            if (Hrs == 0) Time.replace(0, 2, "12"); // Midnight = 12 AM
            Time += " AM";
        }
        return "Posted on " + Date + " at " + Time;
    }
};
