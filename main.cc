#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include "fuse/fuse_main.h"
#include "meta/metafs_builder.h"
#include "meta/meta_file_system.h"
#include "network/chunk_downloader.h"

int main(int argc, char *argv[]) {
    ChunkStore store("/tmp/lol");
    ChunkBuilder chunkBuilder(store);
    MetaFSBuilder builder("/usr/bin");

    auto err = builder.build(chunkBuilder);
    if (err == ErrorCode::OK) {
        std::cout << "OK\n";
    } else {
        std::cout << "ERR\n";
    }


    ChunkDownloader client;
    ChunkProvider chunkProvider(client, store, 128);
    DataProvider dataProvider(chunkProvider);
    MetaFileSystem metaFileSystem(dataProvider);

    run_fuse("/home/adam/mnt/", true, &metaFileSystem, &dataProvider);


    return 0;
}
