#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include "fuse/fuse_main.h"
#include "meta/metafs_builder.h"
#include "meta/meta_file_system.h"

int main(int argc, char *argv[]) {
    MetaFSBuilder builder("/home/adam");
    auto[x, y] = builder.create();
    char *f = x.get();

    MetaFileSystem fs(f);

    std::cout << y << std::endl;

    run_fuse(argc, argv, &fs);
//    // fs.get_dir("/bin", [](const char *p, const struct stat *s) -> void { printf("%s\n", p); });
//    char buff[XATTR_SIZE_MAX];
//    ssize_t s;
//    //fs.list_xattr("/tcslinux.sh", buff, XATTR_SIZE_MAX, &s);
//    fs.get_xattr("/tcslinux.sh", "user.xdg.origin.url", buff, XATTR_SIZE_MAX, &s);
//    std::cout << std::string(buff, s) << std::endl;

    return 0;
}
