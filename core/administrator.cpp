#include "administrator.h"
#include <iostream>
using namespace std;

Administrator::Administrator(const string &accountId, const string &name, const string &password) : User(accountId, name, password) {}
void Administrator::showMenu() const
{
    cout << "\n===== 管理员菜单 =====\n";
    cout << "1. 查看待审核记录\n";
    cout << "2. 审核志愿记录\n";
    cout << "3. 查看全部志愿记录\n";
    cout << "4. 查看排行榜\n";
    cout << "5. 创建学生账号\n";
    cout << "6. 创建管理员账号\n";
    cout << "7. 查看全部学生账号\n";
    cout << "8. 查看全部管理员账号\n";
    cout << "9. 按学生账号查询志愿记录\n";
    cout << "0. 退出登录\n";
}