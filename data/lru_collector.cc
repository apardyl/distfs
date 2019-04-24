#include "lru_collector.h"

LruCollector::LruCollector(uint32_t size, std::function<void(uint32_t)> remover) : size(size),
                                                                                   remover(std::move(remover)) {
}

void LruCollector::update(uint32_t id) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = iterators.find(id);
    if (it == iterators.end()) {
        uint32_t last = queue.back();
        queue.pop_back();
        iterators.erase(last);
        remover(last);
    } else {
        iterators.erase(it);
    }
    queue.push_front(id);
    iterators[id] = queue.begin();
}
