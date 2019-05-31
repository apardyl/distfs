#ifndef DISTFS_LRUCOLLECTOR_H
#define DISTFS_LRUCOLLECTOR_H


#include <functional>
#include <list>
#include <map>
#include <mutex>

class LruCollector {
private:
    uint32_t size;
    std::function<void(uint32_t)> remover;

    std::list<uint32_t> queue;
    std::map<uint32_t, std::list<uint32_t>::iterator> iterators;

    std::mutex mutex;
public:
    LruCollector(uint32_t size, std::function<void(uint32_t)> remover);

    void update(uint32_t id);

    void remove(uint32_t id);
};


#endif //DISTFS_LRUCOLLECTOR_H
