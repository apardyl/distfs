#include "distfs_server.h"
#include "distfs_service.h"

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpcpp/security/server_credentials.h>

DistfsServer::DistfsServer(ChunkStore &chunkStore, ConnectionPool &connectionPool)
        : chunkStore(chunkStore), connectionPool(connectionPool) {}

void DistfsServer::run_server_blocking(const std::string &listen_on) {
    DistfsService service(chunkStore, connectionPool);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(listen_on, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    printf("Listening on %s\n", listen_on.c_str());
    server->Wait();
}
