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

    std::tuple <ErrorCode, uint32_t> get_node_offset(const char *path) const;

    void get_stat(uint32_t offset, struct stat *st) const;

    uint32_t get_child(uint32_t offset, const std::string &name) const;

    uint32_t find_entity(const char *name, const Entry *ents, uint32_t ents_size) const;

public:
    explicit MetaFileSystem(char *data);

    void set_data(char *fs_data);

    ErrorCode get_file_position(const char *path, uint32_t *file_offset, uint32_t *file_length) const;

    ErrorCode get_symlink(const char *path, size_t buff_size, char *buff) const;

    ErrorCode get_stat(const char *path, struct stat *st) const;

    ErrorCode get_dir(const char *path, uint32_t start_at,
                      const std::function<bool(const char *, const struct stat *)> &dir_reader) const;

    ErrorCode list_xattr(const char *path, char *list, size_t buff_size, ssize_t *length) const;

    ErrorCode get_xattr(const char *path, const char *name, void *value, size_t buff_size, ssize_t *length) const;
};


#endif //DISTFS_METAFILESYSTEM_H
