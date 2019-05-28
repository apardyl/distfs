#include <algorithm>
#include "data_provider.h"
#include "../common/consts.h"

ErrorCode DataProvider::read(usize offset, char *buffer, usize length) {
    uint32_t id = offset / CHUNK_SIZE_BYTES;
    usize of = offset % CHUNK_SIZE_BYTES;
    while (length > 0) {
        usize len = std::min(length, static_cast<usize>(CHUNK_SIZE_BYTES));
        uint32_t read_size;
        auto err = chunkProvider.read_chunk(id, buffer, of, len, &read_size);
        if (read_size != len) {
            return ErrorCode::INTERNAL_ERROR;
        }
        if (err != ErrorCode::OK) {
            return err;
        }
        of = 0;
        length -= len;
        buffer += len;
        id += 1;
    }

    return ErrorCode::OK;
}

DataProvider::DataProvider(ChunkProvider &chunkProvider) : chunkProvider(chunkProvider) {}
