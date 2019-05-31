#include <utility>

#include "chunk_store.h"
#include "../common/consts.h"
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

ChunkStore::ChunkStore(std::string store_path, uint32_t size) : base_path(std::move(store_path)) {
    DIR *d, *e, *f;
    dirent *ent;
    if ((d = opendir(base_path.c_str())) != nullptr) {
        while ((ent = readdir(d)) != nullptr) {
            auto l = base_path + '/' + std::string(ent->d_name);
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 &&
                (e = opendir(l.c_str())) != nullptr) {
                while ((ent = readdir(e)) != nullptr) {
                    auto m = l + '/' + std::string(ent->d_name);
                    if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 &&
                        (f = opendir(m.c_str())) != nullptr) {
                        while ((ent = readdir(f)) != nullptr) {
                            char *ptr = nullptr;
                            uint32_t id = strtol(ent->d_name, &ptr, 16);
                            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 && ptr != nullptr &&
                                *ptr == '\0') {
                                in_store.insert(id);
                            }
                        }
                        closedir(f);
                    }
                }
                closedir(e);
            }
        }
        closedir(d);
    }
    collector = std::make_unique<LruCollector>(size, [this](uint32_t id) -> void { this->remove_chunk(id); });
    for (auto id : in_store) {
        collector->update(id);
    }
}

void ChunkStore::id_to_path(uint32_t id, char *path_buf) {
    snprintf(path_buf, PATH_MAX, "%s/%X/%X/%X", base_path.c_str(), id & 0xFF, (id >> 8) & 0xFF, id);
}

ErrorCode ChunkStore::read_chunk(uint32_t id, char *buffer, uint32_t *size) {
    char buf[PATH_MAX];
    id_to_path(id, buf);
    int fd = open(buf, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            return ErrorCode::NOT_FOUND;
        }
        return ErrorCode::INTERNAL_ERROR;
    }

    collector->update(id);

    ssize_t res = 0;
    *size = 0;
    uint32_t max_size = CHUNK_SIZE_BYTES;
    do {
        res = read(fd, buffer, max_size);
        max_size -= res;
        buffer += res;
        *size += res;
    } while (max_size > 0 && res > 0);
    if (res == -1) {
        close(fd);
        return ErrorCode::INTERNAL_ERROR;
    }
    close(fd);
    return ErrorCode::OK;
}

ErrorCode ChunkStore::write_chunk(uint32_t id, char *buffer, uint32_t size) {
    char buf[PATH_MAX];
    id_to_path(id, buf);

    auto err = create_base_dir(buf);
    if (err != ErrorCode::OK) {
        return err;
    }

    int fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        printf("%d\n", errno);
        return ErrorCode::INTERNAL_ERROR;
    }
    ssize_t res = 0;
    do {
        res = write(fd, buffer, size);
        size -= res;
        buffer += res;
    } while (size > 0 && res > 0);
    if (res == -1) {
        close(fd);
        return ErrorCode::INTERNAL_ERROR;
    }
    close(fd);
    collector->update(id);
    {
        std::lock_guard<std::mutex> guard(store_mutex);
        in_store.insert(id);
    }
    return ErrorCode::OK;
}

ErrorCode ChunkStore::remove_chunk(uint32_t id) {
    char buf[PATH_MAX];
    id_to_path(id, buf);
    if (unlink(buf) == -1) {
        if (errno == ENOENT) {
            return ErrorCode::NOT_FOUND;
        }
        return ErrorCode::INTERNAL_ERROR;
    }
    {
        std::lock_guard<std::mutex> guard(store_mutex);
        in_store.erase(id);
    }
    return ErrorCode::OK;
}

std::set<uint32_t> ChunkStore::available() const {
    return in_store;
}

ErrorCode ChunkStore::create_base_dir(const char *path_buf) {
    char buff[FILENAME_MAX];
    char *b = buff;
    while (path_buf != nullptr) {
        const char *next = strchr(path_buf + 1, '/');
        if (next != nullptr) {
            strncpy(b, path_buf, next - path_buf);
            b += next - path_buf;
            *b = '\0';
            int res = mkdir(buff, S_IRWXU);
            if (res != 0 && errno != EEXIST) {
                return ErrorCode::INTERNAL_ERROR;
            }
        }
        path_buf = next;
    }
    return ErrorCode::OK;
}
