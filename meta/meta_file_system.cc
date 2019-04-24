#include "meta_file_system.h"
#include <cstring>

MetaFileSystem::MetaFileSystem(char *data) : data(data) {
}

void MetaFileSystem::set_data(char *data) {
    this->data = data;
}

ErrorCode MetaFileSystem::get_file_position(const char *path, uint32_t *file_offset, uint32_t *file_length) {
    auto[code, offset] = get_node_offset(path);
    if (code == ErrorCode::OK) {
        Node *n = reinterpret_cast<Node *>(offset + data);
        *file_offset = n->data_offset;
        *file_length = n->length;
    }
    return code;
}

ErrorCode MetaFileSystem::get_symlink(const char *path, size_t buff_size, char *buff, ssize_t *length) {
    auto[code, offset] = get_node_offset(path);
    if (code == ErrorCode::OK) {
        Node *n = reinterpret_cast<Node *>(offset + data);
        if (std::strlen(data + n->data_offset) > buff_size) {
            return ErrorCode::TOO_LONG;
        }
        std::strncpy(buff, data + n->data_offset, buff_size);
    }
    return code;
}

ErrorCode MetaFileSystem::get_stat(const char *path, struct stat *st) {
    auto[code, offset] = get_node_offset(path);
    if (code == ErrorCode::OK) {
        get_stat(offset, st);
    }
    return code;
}

ErrorCode MetaFileSystem::get_dir(const char *path, std::function<void(const char *, const struct stat *)> dir_reader) {
    auto[code, offset] = get_node_offset(path);
    if (code == ErrorCode::OK) {
        Node *n = reinterpret_cast<Node *>(offset + data);
        auto *dirents = reinterpret_cast<Entry *>(n->data_offset + data);
        for (unsigned i = 0; i < n->length; i++) {
            struct stat st{};
            get_stat(dirents[i].data_offset, &st);
            dir_reader(data + dirents[i].name_offset, &st);
        }
    }
    return code;
}


ErrorCode MetaFileSystem::list_xattr(const char *path, char *list, size_t buff_size, ssize_t *length) {
    auto[code, offset] = get_node_offset(path);
    if (code == ErrorCode::OK) {
        Node *n = reinterpret_cast<Node *>(offset + data);
        offset += sizeof(Node);
        auto *entries = reinterpret_cast<Entry *>(data + offset);
        for (unsigned i = 0; i < n->xattrs_count; i++) {
            size_t res = strlen(data + entries[i].name_offset) + 1;
            if (res > buff_size) {
                return ErrorCode::TOO_LONG;
            }
            strncpy(list, data + entries[i].name_offset, buff_size);
            buff_size -= res;
            list += res;
            *length += res;
        }
        return ErrorCode::NOT_IMPLEMENTED;
    }
    return code;
}

ErrorCode
MetaFileSystem::get_xattr(const char *path, const char *name, void *value, size_t buff_size, ssize_t *length) {
    auto[code, offset] = get_node_offset(path);
    if (code == ErrorCode::OK) {
        Node *n = reinterpret_cast<Node *>(offset + data);
        offset += sizeof(Node);
        auto *entries = reinterpret_cast<Entry *>(data + offset);
        uint32_t data_offset = find_entity(name, entries, n->xattrs_count);
        if (data_offset == 0) {
            return ErrorCode::NO_DATA;
        }
        uint32_t data_size = *reinterpret_cast<uint32_t *>(data + data_offset);
        if (data_size > buff_size) {
            return ErrorCode::TOO_LONG;
        }
        memcpy(value, data + data_offset + sizeof(uint32_t), data_size);
        *length = data_size;
    }
    return code;
}

void MetaFileSystem::get_stat(uint32_t offset, struct stat *st) {
    Node *n = reinterpret_cast<Node *>(offset + data);
    st->st_mode = n->mode;
    st->st_nlink = S_ISDIR(n->mode) ? 2 : 1;
    st->st_uid = n->uid;
    st->st_gid = n->gid;
    st->st_size = n->size;
    st->st_blocks = n->block_count;
    st->st_atim.tv_nsec = n->mtime_ns % 1000000000; // use mtime as atime
    st->st_atim.tv_sec = n->mtime_ns / 1000000000;
    st->st_mtim.tv_nsec = n->mtime_ns % 1000000000;
    st->st_mtim.tv_sec = n->mtime_ns / 1000000000;
    st->st_ctim.tv_nsec = n->ctime_ns % 1000000000;
    st->st_ctim.tv_sec = n->ctime_ns / 1000000000;
}


std::tuple<ErrorCode, uint32_t> MetaFileSystem::get_node_offset(const char *path) {
    if (*path != '/') {
        throw std::runtime_error("invalid path: " + std::string(path));
    }
    // Skip metadata size
    uint32_t offset = sizeof(uint32_t);
    path++;
    std::string name;
    for (; *path != '\0'; path++) {
        if (*path == '/') {
            auto n = get_child(offset, name);
            name.clear();
            if (n == 0) {
                return std::make_tuple(ErrorCode::NOT_FOUND, 0);
            } else {
                offset = n;
            }
        } else {
            name.push_back(*path);
        }
    }
    if (!name.empty()) {
        auto n = get_child(offset, name);
        if (n == 0) {
            return std::make_tuple(ErrorCode::NOT_FOUND, 0);
        } else {
            offset = n;
        }
    }
    return std::make_tuple(ErrorCode::OK, offset);
}

uint32_t MetaFileSystem::get_child(uint32_t offset, const std::string &name) {
    Node *n = reinterpret_cast<Node *>(offset + data);
    if (!S_ISDIR(n->mode)) {
        return 0;
    }
    auto *dirents = reinterpret_cast<Entry *>(offset + data + sizeof(Node));

    return find_entity(name.c_str(), dirents, n->length);
}

uint32_t MetaFileSystem::find_entity(const char *name, const Entry *ents, uint32_t ents_size) {
    int a = 0, b = ents_size - 1;

    while (a < b) {
        int m = (a + b) / 2;
        int x = std::strcmp(data + ents[m].name_offset, name);
        if (x == 0) {
            return ents[m].data_offset;
        } else if (x < 0) {
            a = m + 1;
        } else {
            b = m - 1;
        }
    }
    if (std::strcmp(data + ents[a].name_offset, name) == 0) {
        return ents[a].data_offset;
    }
    return 0;
}
