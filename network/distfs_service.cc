#include "distfs_service.h"
#include "../common/consts.h"
#include "chunk_availability.h"
#include <grpc/grpc.h>
#include <grpc/impl/codegen/compression_types.h>

using namespace distfs;

grpc::Status DistfsService::GetChunk(::grpc::ServerContext *context, const ::distfs::ChunkRequest *request,
                                     ::grpc::ServerWriter<::distfs::ChunkPartResponse> *writer) {
    debug_print("Chunk request from %s for %d\n", context->peer().c_str(), request->id());
    context->set_compression_algorithm(GRPC_COMPRESS_NONE);
    auto buff = std::unique_ptr<char>(new char[CHUNK_SIZE_BYTES]);
    uint32_t size;
    auto err = store.read_chunk(request->id(), buff.get(), &size);
    if (err == ErrorCode::NOT_FOUND) {
        debug_print("Chunk not found, request from %s for %d\n", context->peer().c_str(), request->id());
        return grpc::Status(grpc::NOT_FOUND, "not found");
    } else if (err != ErrorCode::OK) {
        debug_print("Chunk store error, request from %s for %d\n", context->peer().c_str(), request->id());
        return grpc::Status(grpc::INTERNAL, "");
    }

    uint32_t offset = 0;
    ChunkPartResponse response;
    while (offset < size) {
        response.set_data(buff.get() + offset, std::min(static_cast<uint32_t > (WIRE_CHUNK_SIZE_BYTES), size - offset));
        if (!writer->Write(response)) {
            debug_print("Chunk request cancelled from %s for %d\n", context->peer().c_str(), request->id());
            return grpc::Status::CANCELLED;
        }
        offset += std::min(static_cast<uint32_t > (WIRE_CHUNK_SIZE_BYTES), size - offset);
    }

    debug_print("Uploaded to %s chunk %d\n", context->peer().c_str(), request->id());
    return grpc::Status::OK;
}

grpc::Status
DistfsService::InfoExchange(::grpc::ServerContext *context, const ::distfs::Info *request, ::distfs::Info *response) {
    if (request->fs_id() != connectionPool.get_fs_id()) {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "fs_id missmach");
    }

    std::string peer = context->peer().substr(context->peer().substr(0, 2) == "ip" ? 5 : 0, std::string::npos);
    std::string p = peer.substr(0, peer.find_last_of(':') + 1) + std::to_string(request->listen_port());
    debug_print("InfoExchange from %s, return %s\n", context->peer().c_str(), p.c_str());
    connectionPool.info_from(p, request->node_id(), ChunkAvailability(request->available()));
    response->set_available(ChunkAvailability(store.available()).bitmask());
    response->set_fs_id(connectionPool.get_fs_id());
    response->set_node_id(connectionPool.get_node_id());
    response->set_listen_port(connectionPool.get_listen_port());
    return grpc::Status::OK;
}

grpc::Status DistfsService::PeerExchange(::grpc::ServerContext *context, const ::distfs::PeerList *request,
                                         ::distfs::PeerList *response) {
    response->set_node_id(connectionPool.get_node_id());
    connectionPool.pex_from(context->peer(), request->node_id(), request->peer(), *response);

    return grpc::Status::OK;
}

DistfsService::DistfsService(ChunkStore &store, ConnectionPool &connectionPool)
        : store(store), connectionPool(connectionPool) {}

grpc::Status DistfsService::GetMetadata(::grpc::ServerContext *context, const ::distfs::Empty *request,
                                        ::distfs::Metadata *response) {
    debug_print("InfoExchange from %s\n", context->peer().c_str());
    response->set_fs_id(connectionPool.get_fs_id());
    response->set_node_id(connectionPool.get_node_id());
    for (auto &s : connectionPool.get_block_hashes()) {
        response->add_block_hash(s);
    }

    return grpc::Status::OK;
}
