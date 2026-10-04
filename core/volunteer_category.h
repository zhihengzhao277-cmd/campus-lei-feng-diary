#ifndef VOLUNTEER_CATEGORY_H
#define VOLUNTEER_CATEGORY_H
#include <string>
using namespace std;

class VolunteerCategory
{
private:
    string categoryId;
    string name;
    double coefficient;

public:
    VolunteerCategory(const string &categoryId, const string &name, double coefficient);
    string getCategoryId() const;
    string getName() const;
    double getCoefficient() const;
    double calculateScore(double duration) const;
};
#endif