#ifndef STUDENT_H
#define STUDENT_H
#include "user.h"
using namespace std;

class Student : public User
{
private:
    string className;
    string major;

public:
    Student(const string &accountId, const string &name, const string &password, const string &className, const string &major);
    string getClassName() const;
    string getMajor() const;
    void showMenu() const override;
};
#endif