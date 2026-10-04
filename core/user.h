#ifndef USER_H
#define USER_H
#include <string>
using namespace std;
class User
{
protected:
    string accountId;
    string name;
    string password;

public:
    User(const string &accountId, const string &name, const string &password);
    virtual ~User() = default;
    string getAccountId() const;
    string getName() const;
    bool checkPassword(const string &input) const;
    string getPassword() const;
    void setPassword(const string &newPassword);
    virtual void showMenu() const = 0;
};
#endif