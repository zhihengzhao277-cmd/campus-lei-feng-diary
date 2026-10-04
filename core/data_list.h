#ifndef DATA_LIST_H
#define DATA_LIST_H

#include <vector>

using namespace std;

template <typename T>
class DataList
{
private:
    vector<T> items;

public:
    void add(const T &item)
    {
        items.push_back(item);
    }

    vector<T> &getItems()
    {
        return items;
    }

    const vector<T> &getItems() const
    {
        return items;
    }

    size_t size() const
    {
        return items.size();
    }

    bool empty() const
    {
        return items.empty();
    }
};

#endif