#include <memory>
#include <cstring>
#include "chunk_provider.h"
#include "../common/consts.h"

ErrorCode ChunkProvider::read_chunk(uint32_t id, char *buffer, uint32_t offset, uint32_t len, uint32_t *read_size) {
    if (offset + len > CHUNK_SIZE_BYTES) {
        return ErrorCode::INTERNAL_ERROR;
    }

    // Try cache first
    std::shared_ptr<CacheEntry> ent;
    CacheEntry *ce = nullptr;
    {
        std::lock_guard<std::mutex> guard(entries_mutex);
        if (entries.count(id) > 0) {
            ent = entries[id];
            ce = ent.get();
        } else {
            ent = std::make_shared<CacheEntry>();
            ent->mutex.lock();
            entries[id] = ent;
        }
    }
    if (ce != nullptr) {
        ce->mutex.lock();
        if (ce->len > 0) {
            ce->mutex.unlock();
            collector->update(id);
            memcpy(buffer, ce->data + offset, std::min(len, ce->len - offset));
            *read_size = std::min(len, ce->len - offset);
            return ErrorCode::OK;
        } else {
            std::lock_guard<std::mutex> guard(entries_mutex);
            ce->mutex.unlock();
            ent = std::make_shared<CacheEntry>();
            ent->mutex.lock();
            entries[id] = ent;
        }
    }

    // Load from storage or network to cache
    auto err = compressedStore.read_chunk(id, ent->data, &ent->len);
    if (err == ErrorCode::NOT_FOUND) {
        err = client.fetch_chunk(id);
        if (err == ErrorCode::OK) {
            err = compressedStore.read_chunk(id, ent->data, &ent->len);
        }
    }

    if (err == ErrorCode::OK) {
        ent->mutex.unlock();
        memcpy(buffer, ent->data + offset, std::min(len, ent->len - offset));
        *read_size = std::min(len, ent->len - offset);
        collector->update(id);
    } else {
        {
            std::lock_guard<std::mutex> guard(entries_mutex);
            entries.erase(id);
            ent->len = 0;
            ent->mutex.unlock();
        }
    }
    return err;
}

void ChunkProvider::remove_chunk(uint32_t id) {
    std::lock_guard<std::mutex> guard(entries_mutex);
    entries.erase(id);
}


ChunkProvider::ChunkProvider(ChunkExternalProvider &client, ChunkStore &chunkStore, uint32_t cache_size)
        : chunkStore(chunkStore), compressedStore(CompressedStore(chunkStore)), client(client) {
    collector = std::make_unique<LruCollector>(cache_size,
                                               [this](uint32_t id) -> void { this->remove_chunk(id); });
}
