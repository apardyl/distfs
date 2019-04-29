#include "chunk_builder.h"


ChunkBuilder::ChunkBuilder(ChunkStore &store) : store(store) {}

ErrorCode ChunkBuilder::build(const char *meta_fs_data) {
    MetaFileSystem mfs(meta_fs_data);

    return ErrorCode::NOT_IMPLEMENTED;
}
