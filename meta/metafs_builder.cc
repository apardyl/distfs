#include <utility>

#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <sys/xattr.h>
#include <queue>
#include "metafs_builder.h"
#include "../common/consts.h"

#define ISSUPPORTED(mode) (S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode))

MetaFSBuilder::MetaFSBuilder(std::string root_path) : root_path(std::move(root_path)) {
}

void MetaFSBuilder::reserve_buffer(usize length) {
    while (data_size < length + current_pos) {
        char *new_data = new char[2 * data_size];
        std::memcpy(new_data, data, data_size);
        delete[] data;
        data = new_data;
        data_size *= 2;
    }
}

void MetaFSBuilder::set_stat(const struct stat &st, Node *n) {
    n->mode = st.st_mode;
    n->uid = st.st_uid;
    n->gid = st.st_gid;
    n->size = static_cast<uint64_t>(st.st_size);
    n->block_count = static_cast<uint64_t>(st.st_blocks);
    n->ctime_ns = static_cast<uint64_t>(st.st_ctim.tv_nsec + st.st_ctim.tv_sec * 1000000000);
    n->mtime_ns = static_cast<uint64_t>(st.st_mtim.tv_nsec + st.st_mtim.tv_sec * 1000000000);
}

void MetaFSBuilder::add_xattrs(const Xattrs &xattrs) {
    auto *entries = reinterpret_cast<Entry *>(data + current_pos);
    current_pos += sizeof(Entry) * xattrs.size();
    for (unsigned i = 0; i < xattrs.size(); i++) {
        entries[i].name_offset = current_pos;
        strncpy(data + current_pos, xattrs[i].first.c_str(), xattrs[i].first.size() + 1);
        current_pos += xattrs[i].first.size() + 1;
        entries[i].data_offset = current_pos;
        *(reinterpret_cast<usize *>(data + current_pos)) = static_cast<usize>(xattrs[i].second.size());
        current_pos += sizeof(usize);
        strncpy(data + current_pos, xattrs[i].second.c_str(), xattrs[i].second.size() + 1);
        current_pos += xattrs[i].second.size() + 1;
    }
}

usize MetaFSBuilder::add_file(const struct stat &st, const Xattrs &xattrs) {
    usize pos = current_pos;
    reserve_buffer(sizeof(Node) + xattrs_len(xattrs));
    Node *n = reinterpret_cast<Node *>(data + current_pos);
    set_stat(st, n);
    current_pos += sizeof(Node);
    n->xattrs_count = static_cast<uint32_t>(xattrs.size());
    add_xattrs(xattrs);
    return pos;
}

usize MetaFSBuilder::add_link(const struct stat &st, const char *target, const Xattrs &xattrs) {
    usize pos = current_pos;
    auto len = static_cast<usize>(std::strlen(target)) + 1;
    reserve_buffer(sizeof(Node) + len + xattrs_len(xattrs));
    Node *n = reinterpret_cast<Node *>(data + current_pos);
    set_stat(st, n);
    current_pos += sizeof(Node);
    n->xattrs_count = static_cast<uint32_t>(xattrs.size());
    add_xattrs(xattrs);
    strncpy(data + current_pos, target, len);
    n->data_offset = current_pos;
    current_pos += len;
    return pos;
}

std::tuple<usize, usize>
MetaFSBuilder::add_dir(const struct stat &st, const std::vector<std::string> &sorted_entries, const Xattrs &xattrs) {
    usize pos = current_pos;
    usize len = 0;
    for (const auto &e : sorted_entries) {
        len += e.size() + 1 + sizeof(Entry);
    }
    reserve_buffer(sizeof(Node) + len + xattrs_len(xattrs));

    Node *n = reinterpret_cast<Node *>(data + current_pos);
    set_stat(st, n);
    n->length = static_cast<usize>(sorted_entries.size());
    current_pos += sizeof(Node);
    n->xattrs_count = static_cast<uint32_t>(xattrs.size());
    add_xattrs(xattrs);
    n->data_offset = current_pos;
    auto dirent = reinterpret_cast<Entry *>(data + n->data_offset);
    current_pos += sizeof(Entry) * sorted_entries.size();

    for (unsigned int i = 0; i < sorted_entries.size(); i++) {
        strncpy(data + current_pos, sorted_entries[i].c_str(), sorted_entries[i].size() + 1);
        dirent[i].name_offset = current_pos;
        current_pos += sorted_entries[i].size() + 1;
    }

    return std::make_tuple(pos, n->data_offset);
}

usize MetaFSBuilder::scan_dfs(const std::string &path) {
    struct stat st{};

    if (lstat(path.c_str(), &st) != 0 || !ISSUPPORTED(st.st_mode)) {
        return 0;
    }

    Xattrs xattrs;
    read_xattrs(path, &xattrs);

    if (S_ISLNK(st.st_mode)) {
        char name[PATH_MAX + 1];
        ssize_t res = readlink(path.c_str(), name, PATH_MAX);
        if (res == -1) {
            name[0] = '\0';
        } else {
            name[res] = '\0';
        }
        return add_link(st, name, xattrs);
    }
    if (S_ISREG(st.st_mode)) {
        return add_file(st, xattrs);
    }
    if (S_ISDIR(st.st_mode)) {
        std::vector<std::string> entries;
        DIR *dir = opendir(path.c_str());
        dirent *d;
        while (dir != nullptr && (d = readdir(dir))) {
            if ((strcmp(d->d_name, ".") == 0) || (strcmp(d->d_name, "..") == 0)) {
                continue;
            }
            struct stat n_st{};
            int res = lstat((path + '/' + d->d_name).c_str(), &n_st);
            if (res != -1 && ISSUPPORTED(n_st.st_mode)) {
                entries.emplace_back(d->d_name);
            }

        }
        closedir(dir);
        std::sort(entries.begin(), entries.end());
        auto[addr, dirents] = add_dir(st, entries, xattrs);

        for (unsigned int i = 0; i < entries.size(); i++) {
            set_dirent_node_offset(dirents, i, scan_dfs(path + '/' + entries[i]));
        }

        return addr;
    }
    return 0;
}

void MetaFSBuilder::set_dirent_node_offset(usize dirent_pos, int pos, usize value) {
    reinterpret_cast<Entry *>(data + dirent_pos)[pos].data_offset = value;
}

void MetaFSBuilder::read_xattrs(const std::string &path, Xattrs *xattrs) {
    char buff[std::max(XATTR_LIST_MAX, XATTR_SIZE_MAX)];
    ssize_t res = llistxattr(path.c_str(), buff, XATTR_LIST_MAX);
    if (res == -1) {
        return;
    }
    for (size_t i = 0; i < static_cast<size_t>(res);) {
        xattrs->emplace_back(buff + i, "");
        i += strnlen(buff + i, XATTR_LIST_MAX - i) + 1;
    }
    for (auto &x : *xattrs) {
        res = getxattr(path.c_str(), x.first.c_str(), buff, XATTR_SIZE_MAX);
        if (res != -1) {
            x.second = std::string(buff, static_cast<unsigned long>(res));
        }
    }
    std::sort(xattrs->begin(), xattrs->end());
}

usize MetaFSBuilder::xattrs_len(const Xattrs &xattrs) {
    usize sum = 0;
    for (auto &x : xattrs) {
        sum += x.first.size() + 1 + x.second.size() + 1 + sizeof(Entry) + sizeof(usize);
    }
    return sum;
}

ErrorCode MetaFSBuilder::create_chunks(ChunkBuilder &chunkBuilder) {
    usize data_of;
    auto err = chunkBuilder.reserve(current_pos, &data_of);
    if (err != ErrorCode::OK) {
        return err;
    }

    std::queue<std::pair<usize, std::string>> Q;
    Q.emplace(sizeof(usize), root_path);

    while (!Q.empty()) {
        auto a = Q.front();
        Q.pop();
        Node *n = reinterpret_cast<Node *>(data + a.first);
        if (S_ISREG(n->mode)) {
            n->data_offset = -1;
            n->length = 0;
            err = chunkBuilder.add_file(a.second, &(n->data_offset), &(n->length));
            if (err != ErrorCode::OK) {
                n->data_offset = -1;
                n->length = 0;
                if (err != ErrorCode::NOT_FOUND && err != ErrorCode::ACCESS_DENIED) {
                    return err;
                }
            }
        } else if (S_ISDIR(n->mode)) {
            auto dirent = reinterpret_cast<Entry *>(data + n->data_offset);
            for (usize i = 0; i < n->length; i++) {
                Q.emplace(dirent[i].data_offset, a.second + "/" + std::string(data + dirent[i].name_offset));
            }
        }
    }

    chunkBuilder.write(data_of, data, current_pos);
    return ErrorCode::OK;
}

ErrorCode MetaFSBuilder::build(ChunkBuilder &chunkBuilder) {
    data = new char[CHUNK_SIZE_BYTES];
    data_size = CHUNK_SIZE_BYTES;

    // leave place for metadata length
    current_pos = sizeof(usize);
    scan_dfs(root_path);
    // write length
    *(reinterpret_cast<usize *>(data)) = current_pos;

    auto err = create_chunks(chunkBuilder);
    if (err == ErrorCode::OK) {
        chunkBuilder.build();
    }

    data = nullptr;
    data_size = 0;
    current_pos = 0;
    return err;
}
