#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include "fuse/fuse_main.h"
#include "meta/metafs_builder.h"
#include "meta/meta_file_system.h"

int main(int argc, char *argv[]) {
    MetaFSBuilder builder("/home/adam");
    auto[x, y] = builder.build();
    char *f = x.get();

    MetaFileSystem fs(f);

    std::cout << y << std::endl;

    run_fuse(argv[1], false, &fs);

    return 0;
}
