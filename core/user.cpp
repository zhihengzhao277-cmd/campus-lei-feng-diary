#include "user.h"
using namespace std;

User::User(const string &accountId, const string &name, const string &password) : accountId(accountId), name(name), password(password) {}
string User::getAccountId() const
{
    return accountId;
}
string User::getName() const
{
    return name;
}
bool User::checkPassword(const string &input) const
{
    return password == input;
}
string User::getPassword() const
{
    return password;
}
void User::setPassword(const string &newPassword)
{
    password = newPassword;
}