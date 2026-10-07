#ifndef STUDENT_H
#define STUDENT_H
#include "user.h"
#include <string>

class Student : public User
{
private:
    std::string className;
    std::string major;

public:
    Student(const std::string &accountId, const std::string &name, const std::string &password, const std::string &className, const std::string &major);
    std::string getClassName() const;
    std::string getMajor() const;
    void showMenu() const override;
};
#endif