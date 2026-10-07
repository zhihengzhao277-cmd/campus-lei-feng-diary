#ifndef DATA_LIST_H
#define DATA_LIST_H

#include <cstddef>
#include <vector>

template <typename T>
class DataList
{
private:
    std::vector<T> items;

public:
    void add(const T &item)
    {
        items.push_back(item);
    }

    std::vector<T> &getItems()
    {
        return items;
    }

    const std::vector<T> &getItems() const
    {
        return items;
    }

    std::size_t size() const
    {
        return items.size();
    }

    bool empty() const
    {
        return items.empty();
    }
};

#endif