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
        DistfsMetadata metadata;
        ConnectionPool connectionPool(metadata, store, 20, 200, true);
        DistfsServer server(store, connectionPool);
        server.run_server_blocking(std::string("0.0.0.0:") + std::string(argv[4]));
    }

    if (argc == 7 && strcmp(argv[1], "mount") == 0) {
        ChunkStore store(argv[2], std::stoi(argv[3]));
        DistfsBootstrap distfsBootstrap;
        DistfsMetadata metadata = distfsBootstrap.fetch_metadata(argv[5]);
        ConnectionPool connectionPool(metadata, store, 20, 200, true);
        DistfsServer server(store, connectionPool);
        auto server_thread = std::thread(
                [&server, &argv]() -> void {
                    server.run_server_blocking(std::string("0.0.0.0:") + std::string(argv[4]));
                });
        ChunkProvider chunkProvider(connectionPool, store, 25);
        DataProvider dataProvider(chunkProvider);
        MetaFileSystem metaFileSystem(dataProvider);
        return run_fuse(argv[6], true, &metaFileSystem, &dataProvider);
    }

    printf("Usage:\n"
           "Create a new distfs file system from a directory:\n"
           "    distfs build <path to distfs cache> <path to source directory>\n"
           "Server-only mode:\n"
           "    distfs serve <path to distfs cache> <port to listen on>\n"
           "Mount distfs:\n"
           "    distfs mount <path to distfs cache> <cache size in 4MB block number> <port to listen on> <known distfs node> <mount path>\n");
    return -1;
}
