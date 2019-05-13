#ifndef DISTFS_DATA_PROVIDER_H
#define DISTFS_DATA_PROVIDER_H


#include "../common/error_code.h"
#include "chunk_provider.h"
#include "../common/consts.h"

class DataProvider {
    ChunkProvider &chunkProvider;

public:
    explicit DataProvider(ChunkProvider &chunkProvider);

    ErrorCode read(usize offset, char *buffer, usize length);

};


#endif //DISTFS_DATA_PROVIDER_H
