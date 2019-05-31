#ifndef DISTFS_DISTFS_SERVER_H
#define DISTFS_DISTFS_SERVER_H


#include "../data/chunk_store.h"
#include "connection_pool.h"

class DistfsServer {
    ChunkStore& chunkStore;

    ConnectionPool& connectionPool;
public:

    explicit DistfsServer(ChunkStore &chunkStore, ConnectionPool &connectionPool);

    void run_server_blocking(const std::string &listen_on);

};


#endif //DISTFS_DISTFS_SERVER_H
