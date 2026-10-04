#include "volunteer_category.h"
using namespace std;

VolunteerCategory::VolunteerCategory(const string &categoryId, const string &name, double coefficient) : categoryId(categoryId), name(name), coefficient(coefficient) {}
string VolunteerCategory::getCategoryId() const
{
    return categoryId;
}
string VolunteerCategory::getName() const
{
    return name;
}
double VolunteerCategory::getCoefficient() const
{
    return coefficient;
}
double VolunteerCategory::calculateScore(double duration) const
{
    return duration * coefficient;
}