#ifndef DISTFS_CHUNKBUILDER_H
#define DISTFS_CHUNKBUILDER_H


#include "chunk_store.h"
#include "../meta/meta_file_system.h"

class ChunkBuilder {
    ChunkStore &store;
public:
    explicit ChunkBuilder(ChunkStore &store);

    ErrorCode build(const char *meta_fs_data);
};


#endif //DISTFS_CHUNKBUILDER_H
