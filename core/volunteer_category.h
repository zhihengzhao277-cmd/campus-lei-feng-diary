#ifndef VOLUNTEER_CATEGORY_H
#define VOLUNTEER_CATEGORY_H
#include <string>

class VolunteerCategory
{
private:
    std::string categoryId;
    std::string name;
    double coefficient;

public:
    VolunteerCategory(const std::string &categoryId, const std::string &name, double coefficient);
    std::string getCategoryId() const;
    std::string getName() const;
    double getCoefficient() const;
    double calculateScore(double duration) const;
};
#endif