#ifndef DISTFS_ORDERED_SET_H
#define DISTFS_ORDERED_SET_H

#include <unordered_set>
#include <queue>


class OrderedSet {
    std::unordered_set<std::string> data;
    std::queue<std::unordered_set<std::string>::iterator> iterators;
    uint32_t max_size;
public:
    OrderedSet(uint32_t maxSize);

    bool push(const std::string& str);

    std::string pop();

    uint32_t size();
};


#endif //DISTFS_ORDERED_SET_H
