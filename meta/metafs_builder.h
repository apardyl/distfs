#ifndef DISTFS_METAFSBUILDER_H
#define DISTFS_METAFSBUILDER_H

#include <memory>
#include <vector>
#include "../common/error_code.h"
#include "types.h"
#include <sys/stat.h>

using Xattrs = std::vector<std::pair<std::string, std::string>>;

class MetaFSBuilder {
    std::string root_path;
    char *data;
    uint32_t data_size;
    uint32_t current_pos = 0;

    static void set_stat(const struct stat &st, Node *n);

    void add_xattrs(const Xattrs &xattrs);

    static void read_xattrs(const std::string &path, Xattrs *xattrs);

    static uint32_t xattrs_len(const Xattrs &xattrs);

    uint32_t add_file(const struct stat &st, const Xattrs &xattrs);

    uint32_t add_link(const struct stat &st, const char *target, const Xattrs &xattrs);

    std::tuple<uint32_t, uint32_t>
    add_dir(const struct stat &st, const std::vector<std::string> &sorted_entries, const Xattrs &xattrs);

    void set_dirent_node_offset(uint32_t dirent_pos, int pos, uint32_t value);

    void reserve_buffer(uint32_t length);

    uint32_t scan_dfs(const std::string &path);

public:
    explicit MetaFSBuilder(std::string root_path);

    std::tuple<std::unique_ptr<char>, uint32_t> create();
};


#endif //DISTFS_METAFSBUILDER_H
