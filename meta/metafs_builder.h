#ifndef DISTFS_METAFSBUILDER_H
#define DISTFS_METAFSBUILDER_H

#include <memory>
#include <vector>
#include "../common/error_code.h"
#include "types.h"
#include <sys/stat.h>
#include <functional>

using Xattrs = std::vector<std::pair<std::string, std::string>>;

class MetaFSBuilder {
    std::string root_path;
    char *data;
    usize data_size;
    usize current_pos = 0;

    static void set_stat(const struct stat &st, Node *n);

    void add_xattrs(const Xattrs &xattrs);

    static void read_xattrs(const std::string &path, Xattrs *xattrs);

    static usize xattrs_len(const Xattrs &xattrs);

    usize add_file(const struct stat &st, const Xattrs &xattrs);

    usize add_link(const struct stat &st, const char *target, const Xattrs &xattrs);

    std::tuple<usize, usize>
    add_dir(const struct stat &st, const std::vector<std::string> &sorted_entries, const Xattrs &xattrs);

    void set_dirent_node_offset(usize dirent_pos, int pos, usize value);

    void reserve_buffer(usize length);

    usize scan_dfs(const std::string &path);

public:
    explicit MetaFSBuilder(std::string root_path);

    std::tuple<std::unique_ptr<char>, usize> build();

    void set_file_offsets(const std::function<std::pair<usize, usize>(const char *)> &offset_provider);

    usize size();
};


#endif //DISTFS_METAFSBUILDER_H
