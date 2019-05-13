#ifndef DISTFS_TYPES_H
#define DISTFS_TYPES_H

#include <cstdint>
#include "../common/consts.h"

struct Node {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t block_count;
    uint64_t mtime_ns;
    uint64_t ctime_ns;

    uint32_t xattrs_count;
    usize length;
    usize data_offset;
};

struct Entry {
    usize name_offset;
    usize data_offset;
};

#endif //DISTFS_TYPES_H
