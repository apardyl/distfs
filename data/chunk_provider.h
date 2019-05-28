#ifndef DISTFS_CHUNK_PROVIDER_H
#define DISTFS_CHUNK_PROVIDER_H


#include <cstdint>
#include "../common/error_code.h"
#include "chunk_store.h"
#include "../common/consts.h"
#include "chunk_external_provider.h"
#include "compressed_store.h"

class ChunkProvider {
    ChunkStore &chunkStore;
    CompressedStore compressedStore;
    ChunkExternalProvider &client;

    struct CacheEntry {
        char *data;
        uint32_t len;

        std::mutex mutex;

        CacheEntry() : len(0) {
            data = new char[CHUNK_SIZE_BYTES];
        }

        ~CacheEntry() {
            delete[] data;
        }
    };

    std::mutex entries_mutex;
    std::unordered_map<uint32_t, std::shared_ptr<CacheEntry>> entries;

    std::unique_ptr<LruCollector> collector;

    void remove_chunk(uint32_t id);

public:
    ChunkProvider(ChunkExternalProvider &client, ChunkStore &chunkStore, uint32_t cache_size);

    ErrorCode read_chunk(uint32_t id, char *buffer, uint32_t offset, uint32_t len, uint32_t *read_size);
};


#endif //DISTFS_CHUNK_PROVIDER_H
