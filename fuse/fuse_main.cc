#include "fuse_main.h"

#define FUSE_USE_VERSION 34
#define _XOPEN_SOURCE 700
#define _FILE_OFFSET_BITS 64

#include <fuse.h>

static MetaFileSystem *mfs;

static void *distfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    cfg->auto_cache = 1;
    return nullptr;
}

static int distfs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void) fi;
    auto err = mfs->get_stat(path, st);
    return err == ErrorCode::OK ? 0 : -ENOENT;
}

static int distfs_readlink(const char *path, char *buff, size_t buff_size) {
    auto err = mfs->get_symlink(path, buff_size, buff);
    switch (err) {
        case ErrorCode::OK:
            return 0;
        case ErrorCode::TOO_LONG:
            return 0;
        case ErrorCode::BAD_TYPE:
            return -EINVAL;
        default:
            return -ENOENT;
    }
}

static int distfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset,
                          struct fuse_file_info *info, enum fuse_readdir_flags flags) {
    auto err = mfs->get_dir(path, static_cast<uint32_t>(offset),
                            [buffer, &offset, filler](const char *name, const struct stat *st) -> bool {
                                return filler(buffer, name, st, ++offset, static_cast<fuse_fill_dir_flags> (0)) != 0;
                            });
    switch (err) {
        case ErrorCode::OK:
            return 0;
        case ErrorCode::BAD_TYPE:
            return -ENOTDIR;
        default:
            return -ENOENT;
    }
}

static int distfs_getxattr(const char *path, const char *name, char *value,
                           size_t size) {
    ssize_t len;
    auto err = mfs->get_xattr(path, name, value, size, &len);

    switch (err) {
        case ErrorCode::OK:
            return static_cast<int>(len);
        case ErrorCode::TOO_LONG:
            return -ERANGE;
        case ErrorCode::NO_DATA:
            return -ENODATA;
        default:
            return -ENOENT;
    }
}

static int distfs_listattr(const char *path, char *list, size_t size) {
    ssize_t len;
    auto err = mfs->list_xattr(path, list, size, &len);
    switch (err) {
        case ErrorCode::OK:
            return static_cast<int>(len);
        case ErrorCode::TOO_LONG:
            return -ERANGE;
        default:
            return -ENOENT;
    }
}

static int distfs_open(const char *path, struct fuse_file_info *fi) {
    return -EACCES;
}

static int distfs_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    return -EACCES;
}

static int distfs_release(const char *path, struct fuse_file_info *fi) {
    return 0;
}

int run_fuse(char *mount_path, bool single_thread, MetaFileSystem *metaFileSystem) {
    mfs = metaFileSystem;
    umask(0);

    fuse_operations operations{};
    operations.init = distfs_init;
    operations.getattr = distfs_getattr;
    operations.readlink = distfs_readlink;
    operations.readdir = distfs_readdir;
    operations.open = distfs_open;
    operations.read = distfs_read;
    operations.release = distfs_release;
    operations.getxattr = distfs_getxattr;
    operations.listxattr = distfs_listattr;

    char *argv[5];
    argv[0] = const_cast<char *>("distfs");
    argv[1] = const_cast<char *>("-f");
    argv[2] = const_cast<char *>("-oro,allow_other,default_permissions");
    argv[3] = mount_path;
    if (single_thread) {
        argv[4] = const_cast<char *>("-s");
    }

    return fuse_main(single_thread ? 5 : 4, argv, &operations, nullptr);
}
