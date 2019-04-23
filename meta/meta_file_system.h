#ifndef DISTFS_METAFILESYSTEM_H
#define DISTFS_METAFILESYSTEM_H


#include <cstdint>
#include <cstddef>
#include <functional>
#include <sys/stat.h>
#include "../common/error_code.h"
#include "types.h"

class MetaFileSystem {
private:
    char *data;

    std::tuple<ErrorCode, uint32_t> get_node_offset(const char *path);

    void get_stat(uint32_t offset, struct stat *st);

    uint32_t get_child(uint32_t offset, const std::string &name);

public:
    explicit MetaFileSystem(char *data);

    void set_data(char *data);

    ErrorCode get_file_position(const char *path, uint32_t *file_offset, uint32_t *file_length);

    ErrorCode get_symlink(const char *path, size_t buff_size, char *buff);

    ErrorCode get_stat(const char *path, struct stat *st);

    ErrorCode get_dir(const char *path, std::function<void(const char *, const struct stat *)> dir_reader);
};


#endif //DISTFS_METAFILESYSTEM_H
