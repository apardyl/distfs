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

    if (argc == 3 && strcmp(argv[1], "serve") == 0) {
        ChunkStore store(argv[2]);
        DistfsMetadata metadata;
        ConnectionPool connectionPool(metadata, store, 20, 200, true);
        DistfsServer server(store, connectionPool);
        server.run_server_blocking("0.0.0.0:5554");
    }

    if (argc == 5 && strcmp(argv[1], "mount") == 0) {
        ChunkStore store(argv[2]);
        DistfsBootstrap distfsBootstrap;
        DistfsMetadata metadata = distfsBootstrap.fetch_metadata(argv[3]);
        ConnectionPool connectionPool(metadata, store, 20, 200, true);
        DistfsServer server(store, connectionPool);
        auto server_thread = std::thread([&server]() -> void { server.run_server_blocking("0.0.0.0:5555"); });
        ChunkProvider chunkProvider(connectionPool, store, 128);
        DataProvider dataProvider(chunkProvider);
        MetaFileSystem metaFileSystem(dataProvider);
        return run_fuse(argv[4], true, &metaFileSystem, &dataProvider);
    }

    printf("Usage:\n");
    // TODO
    return -1;
}
