#pragma once
#include <iostream>
#include <ctime>
#include <vector>
#include "User.h"
#include "Vote.h"
#include "Comment.h"
using namespace std;

class Student : public User {
private:
    vector<Vote> Votes;

public:

	Student(string username, string password,string email, int id) : User(username, password, email, id) {
	}

	//check if the student has already voted for a course
    int Voted(Vote v) {
		for (int i = 0; i < Votes.size(); i++) {
            if (Votes[i] == v) {
                return i;
            }
		}
        return -1;
    }

    void vote(string CourseName, Teacher* Teacher, int Value) {
		Vote v;
		v.CourseName = CourseName; v.teacher = Teacher; v.Value = Value;
		int Idx = Voted(v);
		// if the student has already voted for this course
        if (Idx != -1) {
			// if the student chose the same vote
            if (v.Value == Value) {
				//remove the vote
				Votes.erase(Votes.begin() + Idx);
				Teacher->ChangeRating(CourseName, -v.Value);
				Teacher->ChangeRatingCount(CourseName, -1);
            }
			// if the student changed his vote
            else {
                //update the vote
				Votes[Idx].Value = v.Value;
				// if the student changed their vote from 1 to 0 
				if (Value == 0)
				{
					Teacher->ChangeRating(CourseName, -1);
				}
				// if the student changed their vote from 0 to 1
				else if (Value == 1)
				{
					Teacher->ChangeRating(CourseName, 1);
				}
			}
		}
		// if the student has not voted for this course
        else {
            Votes.push_back(v);
			Teacher->ChangeRating(CourseName, v.Value);
			Teacher->ChangeRatingCount(CourseName, 1);
        }
    }

	void comment(string CourseName, Teacher* Teacher, string msg) {
		Comment C; C.Content = msg; C.timestamp = time(0);
		Teacher->AddComment(CourseName, C);
	}



};
