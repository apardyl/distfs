#ifndef DISTFS_TYPES_H
#define DISTFS_TYPES_H

#include <cstdint>

struct Node {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t blockcnt;
    uint64_t mtime_ns;
    uint64_t ctime_ns;

    uint32_t length;
    uint32_t data_offset;
};

struct Dirent {
    uint32_t name_offset;
    uint32_t node_offset;
};

#endif //DISTFS_TYPES_H
