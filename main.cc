#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include "meta/metafs_builder.h"
#include "meta/meta_file_system.h"

int main() {
    MetaFSBuilder builder("/");
    auto[x, y] = builder.create();
    char *f = x.get();

    MetaFileSystem fs(f);

    std::cout << y << std::endl;
    fs.get_dir("/usr/bin", [](const char *p, const struct stat *s) -> void { printf("%s\n", p); });

    return 0;
}