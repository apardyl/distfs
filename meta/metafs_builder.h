#ifndef DISTFS_METAFSBUILDER_H
#define DISTFS_METAFSBUILDER_H

#include <memory>
#include <vector>
#include "../common/error_code.h"
#include "types.h"
#include "../data/chunk_builder.h"
#include <sys/stat.h>
#include <functional>

using Xattrs = std::vector<std::pair<std::string, std::string>>;

class MetaFSBuilder {
    std::string root_path = "";
    char *data = nullptr;
    usize data_size = 0;
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

    ErrorCode create_chunks(ChunkBuilder &chunkBuilder);

public:
    explicit MetaFSBuilder(std::string root_path);

    ErrorCode build(ChunkBuilder &chunkBuilder);
};


#endif //DISTFS_METAFSBUILDER_H
