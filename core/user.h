#ifndef USER_H
#define USER_H
#include <string>
class User
{
protected:
    std::string accountId;
    std::string name;
    std::string password;

public:
    User(const std::string &accountId, const std::string &name, const std::string &password);
    virtual ~User() = default;
    std::string getAccountId() const;
    std::string getName() const;
    bool checkPassword(const std::string &input) const;
    std::string getPassword() const;
    void setPassword(const std::string &newPassword);
    virtual void showMenu() const = 0;
};
#endif