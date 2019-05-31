#ifndef DISTFS_CHUNK_AVAILABILITY_H
#define DISTFS_CHUNK_AVAILABILITY_H


#include <cstdint>
#include <set>

class ChunkAvailability {
    std::set<uint32_t> chunks;
public:
    explicit ChunkAvailability(const std::set<uint32_t> &chunks);

    explicit ChunkAvailability(std::set<uint32_t> &&chunks);

    explicit ChunkAvailability(const std::string &bitmask);

    ChunkAvailability();

    bool operator[](uint32_t id) const;

    std::string bitmask() const;

    uint32_t count() const;

    bool operator<(const ChunkAvailability &a) const;
};


#endif //DISTFS_CHUNK_AVAILABILITY_H
