#ifndef DISTFS_CHUNKSTORE_H
#define DISTFS_CHUNKSTORE_H


#include <cstdint>
#include <string>
#include "../common/error_code.h"

class ChunkStore {
private:
    std::string base_path;

    void id_to_path(uint32_t id, char * path_buf);

public:
    explicit ChunkStore(const std::string &store_path);

    ErrorCode read_chunk(uint32_t id, char *buffer, uint32_t size);

    ErrorCode write_chunk(uint32_t id, char *buffer, uint32_t size);

    ErrorCode remove_chunk(uint32_t id);
};


#endif //DISTFS_CHUNKSTORE_H
