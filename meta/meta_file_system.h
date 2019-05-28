#ifndef DISTFS_METAFILESYSTEM_H
#define DISTFS_METAFILESYSTEM_H


#include <cstdint>
#include <cstddef>
#include <memory>
#include <functional>
#include <sys/stat.h>
#include "../common/error_code.h"
#include "types.h"
#include "../data/data_provider.h"

class MetaFileSystem {
private:
    std::shared_ptr<char> data_ptr;
    const char * data;

    std::tuple <ErrorCode, usize> get_node_offset(const char *path) const;

    void get_stat(usize offset, struct stat *st) const;

    usize get_child(usize offset, const std::string &name) const;

    usize find_entity(const char *name, const Entry *ents, uint32_t ents_size) const;

public:
    explicit MetaFileSystem(std::shared_ptr<char> data);

    explicit MetaFileSystem(DataProvider& dataProvider);

    ErrorCode get_file_position(const char *path, usize *file_offset, usize *file_length) const;

    ErrorCode get_file_position(usize offset, usize *file_offset, usize *file_length) const;

    ErrorCode get_symlink(const char *path, size_t buff_size, char *buff) const;

    ErrorCode get_stat(const char *path, struct stat *st) const;

    ErrorCode get_dir(const char *path, uint32_t start_at,
                      const std::function<bool(const char *, const struct stat *)> &dir_reader) const;

    ErrorCode list_xattr(const char *path, char *list, size_t buff_size, ssize_t *length) const;

    ErrorCode get_xattr(const char *path, const char *name, void *value, size_t buff_size, ssize_t *length) const;

    ErrorCode get_offset(const char *path, usize *offset);
};


#endif //DISTFS_METAFILESYSTEM_H
