#ifndef DISTFS_CHUNKSTORE_H
#define DISTFS_CHUNKSTORE_H


#include <cstdint>
#include <string>
#include <set>
#include <mutex>
#include <memory>
#include "../common/error_code.h"
#include "lru_collector.h"

class ChunkStore {
private:
    std::string base_path;

    void id_to_path(uint32_t id, char *path_buf);

    std::set<uint32_t> in_store;

    std::mutex store_mutex;

    ErrorCode create_base_dir(const char *path);

    std::unique_ptr<LruCollector> collector;

public:
    explicit ChunkStore(std::string store_path, uint32_t size = 1 << 31);

    ErrorCode read_chunk(uint32_t id, char *buffer, uint32_t *size);

    ErrorCode write_chunk(uint32_t id, char *buffer, uint32_t size);

    ErrorCode remove_chunk(uint32_t id);

    const std::set<uint32_t> &available() const;
};


#endif //DISTFS_CHUNKSTORE_H
