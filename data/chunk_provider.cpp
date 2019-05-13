#include "chunk_provider.h"
#include "../common/consts.h"

ErrorCode ChunkProvider::read_chunk(uint32_t id, char *buffer, uint32_t offset, uint32_t len) {
    if (offset + len > CHUNK_SIZE_BYTES) {
        return ErrorCode::INTERNAL_ERROR;
    }

    return ErrorCode::NOT_IMPLEMENTED;
}
