#ifndef ADMINISTRATOR_H
#define ADMINISTRATOR_H
#include "user.h"

class Administrator : public User
{
public:
    Administrator(const std::string &accountId, const std::string &name, const std::string &password);
    void showMenu() const override;
};
#endif