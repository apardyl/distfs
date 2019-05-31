#ifndef DISTFS_COMPRESSED_STORE_H
#define DISTFS_COMPRESSED_STORE_H


#include "chunk_store.h"
#include "compression_engine.h"

class CompressedStore {
private:
    ChunkStore &chunkStore;
    CompressionEngine compressionEngine;

public:
    explicit CompressedStore(ChunkStore &chunkStore);

    ErrorCode read_chunk(uint32_t id, char *buffer, uint32_t *size);

    ErrorCode write_chunk(uint32_t id, char *buffer, uint32_t size);

    ErrorCode remove_chunk(uint32_t id);

    std::set<uint32_t> available() const;
};


#endif //DISTFS_COMPRESSED_STORE_H
