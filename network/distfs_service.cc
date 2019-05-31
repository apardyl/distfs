#include "distfs_service.h"
#include "../common/consts.h"
#include "chunk_availability.h"
#include <grpc/grpc.h>
#include <grpc/impl/codegen/compression_types.h>

using namespace distfs;

grpc::Status DistfsService::GetChunk(::grpc::ServerContext *context, const ::distfs::ChunkRequest *request,
                                     ::grpc::ServerWriter<::distfs::ChunkPartResponse> *writer) {
    connectionPool.connection_from(context->peer());

    context->set_compression_algorithm(GRPC_COMPRESS_NONE);
    auto buff = std::unique_ptr<char>(new char[CHUNK_SIZE_BYTES]);
    uint32_t size;
    auto err = store.read_chunk(request->id(), buff.get(), &size);
    if (err == ErrorCode::NOT_FOUND) {
        return grpc::Status(grpc::NOT_FOUND, "not found");
    } else if (err != ErrorCode::OK) {
        return grpc::Status(grpc::INTERNAL, "");
    }

    uint32_t offset = 0;
    ChunkPartResponse response;
    while (offset < size) {
        response.set_data(buff.get() + offset, std::min(static_cast<uint32_t > (WIRE_CHUNK_SIZE_BYTES), size - offset));
        if (!writer->Write(response)) {
            return grpc::Status::CANCELLED;
        }
        offset += std::min(static_cast<uint32_t > (WIRE_CHUNK_SIZE_BYTES), size - offset);
    }

    return grpc::Status::OK;
}

grpc::Status
DistfsService::InfoExchange(::grpc::ServerContext *context, const ::distfs::Info *request, ::distfs::Info *response) {
    connectionPool.connection_from(context->peer());

    if (request->fs_id() != connectionPool.get_fs_id()) {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "fs_id missmach");
    }

    connectionPool.info_from(context->peer(), ChunkAvailability(response->available()));
    response->set_available(ChunkAvailability(store.available()).bitmask());
    response->set_fs_id(connectionPool.get_fs_id());
    response->set_node_id(connectionPool.get_node_id());
    return grpc::Status::OK;
}

grpc::Status DistfsService::PeerExchange(::grpc::ServerContext *context, const ::distfs::PeerList *request,
                                         ::distfs::PeerList *response) {
    connectionPool.connection_from(context->peer());

    connectionPool.pex_from(context->peer(), request->peer(), *response);

    return grpc::Status::OK;
}

grpc::Status
DistfsService::Ping(::grpc::ServerContext *context, const ::distfs::Empty *request, ::distfs::Empty *response) {
    connectionPool.connection_from(context->peer());
    return grpc::Status::OK;
}

DistfsService::DistfsService(ChunkStore &store, ConnectionPool &connectionPool)
        : store(store), connectionPool(connectionPool) {}

grpc::Status DistfsService::GetMetadata(::grpc::ServerContext *context, const ::distfs::Empty *request,
                                        ::distfs::Metadata *response) {
    connectionPool.connection_from(context->peer());
    response->set_fs_id(connectionPool.get_fs_id());
    return grpc::Status::OK;
}
