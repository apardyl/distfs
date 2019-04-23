#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include "metafs_builder.h"
#include "../common/consts.h"

#define ISSUPPORTED(mode) (S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode))

MetaFSBuilder::MetaFSBuilder(const std::string &root_path) : root_path(root_path) {
    data = new char[CHUNK_SIZE_BYTES];
    data_size = CHUNK_SIZE_BYTES;
}

std::tuple<std::unique_ptr<char>, uint32_t> MetaFSBuilder::create() {
    // leave place for metadata length
    current_pos = sizeof(uint32_t);

    scan_dfs(root_path);

    // write length
    *(reinterpret_cast<uint32_t *>(data)) = current_pos;
    return std::make_tuple(std::unique_ptr<char>(data), current_pos);
}

void MetaFSBuilder::reserve_buffer(uint32_t length) {
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
    n->blockcnt = static_cast<uint64_t>(st.st_blocks);
    n->ctime_ns = static_cast<uint64_t>(st.st_ctim.tv_nsec + st.st_ctim.tv_sec * 1000000000);
    n->mtime_ns = static_cast<uint64_t>(st.st_mtim.tv_nsec + st.st_mtim.tv_sec * 1000000000);
}

uint32_t MetaFSBuilder::add_file(const struct stat &st) {
    reserve_buffer(sizeof(Node));
    Node *n = reinterpret_cast<Node *>(data + current_pos);
    set_stat(st, n);
    uint32_t pos = current_pos;
    current_pos += sizeof(Node);
    return pos;
}

uint32_t MetaFSBuilder::add_link(const struct stat &st, const char *target) {
    auto len = static_cast<uint32_t>(std::strlen(target)) + 1;
    reserve_buffer(sizeof(Node) + len);
    Node *n = reinterpret_cast<Node *>(data + current_pos);
    set_stat(st, n);
    strncpy(data + current_pos + sizeof(Node), target, len);
    uint32_t pos = current_pos;
    current_pos += sizeof(Node) + len;
    return pos;
}

std::tuple<uint32_t, uint32_t>
MetaFSBuilder::add_dir(const struct stat &st, const std::vector<std::string> &sorted_entries) {
    uint32_t pos = current_pos;
    uint32_t len = 0;
    for (const auto &e : sorted_entries) {
        len += e.size() + 1 + sizeof(Dirent);
    }
    reserve_buffer(sizeof(Node) + len);

    Node *n = reinterpret_cast<Node *>(data + current_pos);
    set_stat(st, n);
    n->length = static_cast<uint32_t>(sorted_entries.size());
    current_pos += sizeof(Node);
    uint32_t dirent_pos = current_pos;
    auto dirent = reinterpret_cast<Dirent *>(data + dirent_pos);
    current_pos += sizeof(Dirent) * sorted_entries.size();

    for (unsigned int i = 0; i < sorted_entries.size(); i++) {
        strncpy(data + current_pos, sorted_entries[i].c_str(), sorted_entries[i].size() + 1);
        dirent[i].name_offset = current_pos;
        current_pos += sorted_entries[i].size() + 1;
    }

    return std::make_tuple(pos, dirent_pos);
}

uint32_t MetaFSBuilder::scan_dfs(const std::string &path) {
    struct stat st{};
    lstat(path.c_str(), &st);
    if (S_ISLNK(st.st_mode)) {
        char name[PATH_MAX + 1];
        ssize_t res = readlink(path.c_str(), name, PATH_MAX);
        if (res == -1) {
            name[0] = '\0';
        } else {
            name[res] = '\0';
        }
        return add_link(st, name);
    }
    if (S_ISREG(st.st_mode)) {
        return add_file(st);
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
        auto[addr, dirents] = add_dir(st, entries);

        for (unsigned int i = 0; i < entries.size(); i++) {
            set_dirent_node_offset(dirents, i, scan_dfs(path + '/' + entries[i]));
        }

        return addr;
    }
    return 0;
}

void MetaFSBuilder::set_dirent_node_offset(uint32_t dirent_pos, int pos, uint32_t value) {
    reinterpret_cast<Dirent *>(data + dirent_pos)[pos].node_offset = value;
}
