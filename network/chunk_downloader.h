#ifndef DISTFS_CHUNK_DOWNLOADER_H
#define DISTFS_CHUNK_DOWNLOADER_H


#include "../data/chunk_external_provider.h"

class ChunkDownloader : public ChunkExternalProvider {
public:
    ErrorCode fetch_chunk(uint32_t id, ChunkStore& store) override;
};


#endif //DISTFS_CHUNK_DOWNLOADER_H
