#ifndef DISTFS_DISTFS_SERVICE_H
#define DISTFS_DISTFS_SERVICE_H

#include "network/proto/distfs.grpc.pb.h"
#include "../data/chunk_store.h"
#include "connection_pool.h"


class DistfsService : public distfs::DistFS::Service {
    ChunkStore &store;

    ConnectionPool &connectionPool;

public:
    explicit DistfsService(ChunkStore &store, ConnectionPool &connectionPool);

    grpc::Status GetChunk(::grpc::ServerContext *context, const ::distfs::ChunkRequest *request,
                          ::grpc::ServerWriter<::distfs::ChunkPartResponse> *writer) override;

    grpc::Status
    InfoExchange(::grpc::ServerContext *context, const ::distfs::Info *request, ::distfs::Info *response) override;

    grpc::Status PeerExchange(::grpc::ServerContext *context, const ::distfs::PeerList *request,
                              ::distfs::PeerList *response) override;

    grpc::Status
    GetMetadata(::grpc::ServerContext *context, const ::distfs::Empty *request, ::distfs::Metadata *response) override;
};


#endif //DISTFS_DISTFS_SERVICE_H
