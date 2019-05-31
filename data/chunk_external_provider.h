#include <cstdint>
#include "../common/error_code.h"
#include "chunk_store.h"

#ifndef DISTFS_CHUNK_EXTERNAL_PROVIDER_H
#define DISTFS_CHUNK_EXTERNAL_PROVIDER_H

class ChunkExternalProvider {
public:
    virtual ErrorCode fetch_chunk(uint32_t id) = 0;
};

#endif //DISTFS_CHUNK_EXTERNAL_PROVIDER_H
