#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include "fuse/fuse_main.h"
#include "meta/metafs_builder.h"
#include "meta/meta_file_system.h"
#include "network/distfs_server.h"
#include "network/distfs_bootstrap.h"

int main(int argc, char *argv[]) {
    if (argc == 4 && strcmp(argv[1], "build") == 0) {
        ChunkStore store(argv[2]);

        ChunkBuilder chunkBuilder(store);
        MetaFSBuilder builder(argv[3]);

        auto err = builder.build(chunkBuilder);
        if (err == ErrorCode::OK) {
            return 0;
        } else {
            std::cout << "ERROR\n";
            return 1;
        }
    }

    if (argc == 4 && strcmp(argv[1], "serve") == 0) {
        ChunkStore store(argv[2]);
        DistfsMetadata metadata(store);
        uint32_t listen_port = std::stoi(argv[3]);
        ConnectionPool connectionPool(metadata, store, 20, 200, true, listen_port);
        DistfsServer server(store, connectionPool);
        server.run_server_blocking(std::string("0.0.0.0:") + std::to_string(listen_port));
    }

    if (argc == 5 && strcmp(argv[1], "mirror") == 0) {
        ChunkStore store(argv[2]);
        DistfsBootstrap distfsBootstrap;
        DistfsMetadata metadata = distfsBootstrap.fetch_metadata(argv[4]);
        uint32_t listen_port = std::stoi(argv[3]);
        ConnectionPool connectionPool(metadata, store, 20, 200, true, listen_port);
        for (uint32_t id = 0; id < metadata.get_hashes().size(); id++) {
            connectionPool.fetch_chunk(id);
        }
        DistfsServer server(store, connectionPool);
        server.run_server_blocking(std::string("0.0.0.0:") + std::to_string(listen_port));
    }

    if (argc == 7 && strcmp(argv[1], "mount") == 0) {
        ChunkStore store(argv[2], std::stoi(argv[3]));
        DistfsBootstrap distfsBootstrap;
        DistfsMetadata metadata = distfsBootstrap.fetch_metadata(argv[5]);
        uint32_t listen_port = std::stoi(argv[4]);
        ConnectionPool connectionPool(metadata, store, 20, 200, true, listen_port);
        DistfsServer server(store, connectionPool);
        auto server_thread = std::thread(
                [&server, listen_port]() -> void {
                    server.run_server_blocking(std::string("0.0.0.0:") + std::to_string(listen_port));
                });
        ChunkProvider chunkProvider(connectionPool, store, 25);
        DataProvider dataProvider(chunkProvider);
        MetaFileSystem metaFileSystem(dataProvider);
        return run_fuse(argv[6], false, &metaFileSystem, &dataProvider);
    }

    printf("Usage:\n"
           "Create a new distfs file system from a directory:\n"
           "    distfs build <path to distfs cache> <path to source directory>\n"
           "Server-only mode:\n"
           "    distfs serve <path to distfs cache> <port to listen on>\n"
           "Mirror server-only mode:\n"
           "    distfs mirror <path to distfs cache> <port to listen on> <known distfs node>\n"
           "Mount distfs:\n"
           "    distfs mount <path to distfs cache> <cache size in 4MB block number> <port to listen on> <known distfs node> <mount path>\n");
    return -1;
}
