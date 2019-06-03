#include "ordered_set.h"

bool OrderedSet::push(const std::string &str) {
    if (data.count(str) == 0) {
        auto p = data.insert(str);
        iterators.push(p.first);
        while (data.size() > max_size) {
            pop();
        }
        return true;
    }
    return false;
}

std::string OrderedSet::pop() {
    auto it = iterators.front();
    iterators.pop();
    auto str = *it;
    data.erase(it);
    return str;
}

OrderedSet::OrderedSet(uint32_t maxSize) : max_size(maxSize) {}

uint32_t OrderedSet::size() {
    return data.size();
}
