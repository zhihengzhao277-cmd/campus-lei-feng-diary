#include "student.h"
#include <iostream>
using namespace std;

Student::Student(const string &accountId, const string &name, const string &password, const string &className, const string &major) : User(accountId, name, password), className(className), major(major) {}
string Student::getClassName() const
{
    return className;
}
string Student::getMajor() const
{
    return major;
}
void Student::showMenu() const
{
    cout << "\n===== 学生菜单 =====\n";
    cout << "1. 查看个人信息\n";
    cout << "2. 提交志愿记录\n";
    cout << "3. 查看我的志愿记录\n";
    cout << "4. 修改待审核/被驳回记录\n";
    cout << "5. 发布日记\n";
    cout << "6. 查看日记墙\n";
    cout << "7. 点赞日记\n";
    cout << "8. 修改密码\n";
    cout << "9. 查看个人积分\n";
    cout << "10. 查看徽章\n";
    cout << "11. 查看排行榜\n";
    cout << "12. 删除志愿记录\n";
    cout << "0. 退出登录\n";
}