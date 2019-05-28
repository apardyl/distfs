#ifndef DISTFS_CHUNKBUILDER_H
#define DISTFS_CHUNKBUILDER_H


#include "chunk_store.h"
#include "../common/consts.h"
#include "compressed_store.h"

class ChunkBuilder {
    CompressedStore store;

    bool built = false;

    usize pos = 0;

    char *buff;

    char *read_buffer;

    uint32_t chunk_num = 0;

    ErrorCode move_pos(usize new_offset);

    ErrorCode write_data(const char *data, usize length);

public:
    explicit ChunkBuilder(ChunkStore &store);

    virtual ~ChunkBuilder();

    ErrorCode add_file(const std::string &path, usize *offset, usize *length);

    ErrorCode reserve(usize length, usize *offset);

    ErrorCode write(usize offset, const char *data, usize length);

    ErrorCode build();
};


#endif //DISTFS_CHUNKBUILDER_H
