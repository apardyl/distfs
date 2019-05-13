#ifndef DISTFS_CHUNK_PROVIDER_H
#define DISTFS_CHUNK_PROVIDER_H


#include <cstdint>
#include "../common/error_code.h"

class ChunkProvider {
public:
    ErrorCode read_chunk(uint32_t id, char *buffer, uint32_t offset, uint32_t len);
};


#endif //DISTFS_CHUNK_PROVIDER_H
