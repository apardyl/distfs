#ifndef DISTFS_FUSE_MAIN_H
#define DISTFS_FUSE_MAIN_H

#include "../meta/meta_file_system.h"

int run_fuse(char *mount_path, bool single_thread, MetaFileSystem *metaFileSystem);


#endif //DISTFS_FUSE_MAIN_H
