#include "chunk_store.h"
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

ChunkStore::ChunkStore(const std::string &store_path) : base_path(store_path) {
}

void ChunkStore::id_to_path(uint32_t id, char *path_buf) {
    snprintf(path_buf, PATH_MAX, "%s/%X/%X/%X", base_path.c_str(), id & 0xFF, (id >> 8) & 0xFF, id);
}

ErrorCode ChunkStore::read_chunk(uint32_t id, char *buffer, uint32_t size) {
    char buf[PATH_MAX];
    id_to_path(id, buf);
    int fd = open(buf, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            return ErrorCode::NOT_FOUND;
        }
        return ErrorCode::ACCESS_DENIED;
    }

    ssize_t res = 0;
    do {
        res = read(fd, buffer, size);
        size -= res;
        buffer += res;
    } while (size > 0 && res > 0);
    if (res == -1) {
        close(fd);
        return ErrorCode::ACCESS_DENIED;
    }
    close(fd);
    return ErrorCode::OK;
}

ErrorCode ChunkStore::write_chunk(uint32_t id, char *buffer, uint32_t size) {
    char buf[PATH_MAX];
    id_to_path(id, buf);
    int fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        return ErrorCode::ACCESS_DENIED;
    }
    ssize_t res = 0;
    do {
        res = write(fd, buffer, size);
        size -= res;
        buffer += res;
    } while (size > 0 && res > 0);
    if (res == -1) {
        close(fd);
        return ErrorCode::ACCESS_DENIED;
    }
    close(fd);
    return ErrorCode::OK;
}

ErrorCode ChunkStore::remove_chunk(uint32_t id) {
    char buf[PATH_MAX];
    id_to_path(id, buf);
    if (unlink(buf) == -1) {
        if (errno == ENOENT) {
            return ErrorCode::NOT_FOUND;
        }
        return ErrorCode::ACCESS_DENIED;
    }
    return ErrorCode::OK;
}
