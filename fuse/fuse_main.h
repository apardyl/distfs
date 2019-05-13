#ifndef DISTFS_FUSE_MAIN_H
#define DISTFS_FUSE_MAIN_H

#include "../meta/meta_file_system.h"
#include "../data/data_provider.h"

int run_fuse(char *mount_path, bool single_thread, MetaFileSystem *metaFileSystem, DataProvider *dataProvider);


#endif //DISTFS_FUSE_MAIN_H
