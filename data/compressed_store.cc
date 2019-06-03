#include "compressed_store.h"
#include "../common/consts.h"

CompressedStore::CompressedStore(ChunkStore &chunkStore) : chunkStore(chunkStore) {}

ErrorCode CompressedStore::read_chunk(uint32_t id, char *buffer, uint32_t *size) {
    char *buf = new char[CHUNK_SIZE_BYTES];
    uint32_t rd;
    auto err = chunkStore.read_chunk(id, buf, &rd);
    if (err == ErrorCode::OK) {
        int status = compressionEngine.decompress(buffer, CHUNK_SIZE_BYTES, buf, rd);
        if (status < 0) {
            debug_print("CompressedStore: decompression error for %d\n", id);
            err = ErrorCode::INTERNAL_ERROR;
        } else {
            *size = status;
        }
    }
    delete[] buf;
    return err;
}

ErrorCode CompressedStore::write_chunk(uint32_t id, char *buffer, uint32_t size) {
    char *buf = new char[CHUNK_SIZE_BYTES];
    int status = compressionEngine.compress(buf, CHUNK_SIZE_BYTES, buffer, size);
    if (status < 0) {
        debug_print("CompressedStore: compression error for %d\n", id);
        return ErrorCode::INTERNAL_ERROR;
    }
    auto err = chunkStore.write_chunk(id, buf, status);
    delete[] buf;
    return err;
}

ErrorCode CompressedStore::remove_chunk(uint32_t id) {
    return chunkStore.remove_chunk(id);
}

std::set<uint32_t> CompressedStore::available() const {
    return chunkStore.available();
}
