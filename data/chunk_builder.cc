#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "chunk_builder.h"


ChunkBuilder::ChunkBuilder(ChunkStore &store) : store(store) {
    buff = new char[CHUNK_SIZE_BYTES];
    read_buffer = new char[CHUNK_SIZE_BYTES];
}

ErrorCode ChunkBuilder::add_file(const std::string &path, usize *offset, usize *length) {
    *offset = 0;
    *length = 0;
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            return ErrorCode::NOT_FOUND;
        }
        return ErrorCode::ACCESS_DENIED;
    }

    ssize_t res = 0;
    char *buffer = read_buffer;
    uint32_t max_size = CHUNK_SIZE_BYTES;

    do {
        do {
            res = read(fd, buffer, max_size);
            max_size -= res;
            buffer += res;
            *length += res;
        } while (max_size > 0 && res > 0);
        auto err = write_data(read_buffer, CHUNK_SIZE_BYTES - max_size);
        if (err != ErrorCode::OK) {
            close(fd);
            return err;
        }
        buffer = read_buffer;
        max_size = CHUNK_SIZE_BYTES;
    } while (res > 0);

    if (res == -1) {
        close(fd);
        return ErrorCode::ACCESS_DENIED;
    }
    close(fd);
    return ErrorCode::OK;
}

ErrorCode ChunkBuilder::reserve(usize length, usize *offset) {
    *offset = pos;
    return move_pos(pos + length);
}

ErrorCode ChunkBuilder::write(usize offset, const char *data, usize length) {
    auto err = move_pos(offset);
    if (err != ErrorCode::OK) {
        return err;
    }
    return write_data(data, length);
}

ErrorCode ChunkBuilder::write_data(const char *data, usize length) {
    while (length > 0) {
        usize curr = pos % CHUNK_SIZE_BYTES;
        usize next = std::min(CHUNK_SIZE_BYTES - curr, length);
        memcpy(buff + curr, data, next);
        length -= next;
        data += next;
        auto err = move_pos(pos += next);
        if (err != ErrorCode::OK) {
            return err;
        }
    }
    return ErrorCode::OK;
}

ChunkBuilder::~ChunkBuilder() {
    delete[] buff;
    delete[] read_buffer;

    if (!built) {
        for (auto &i : store.available()) {
            store.remove_chunk(i);
        }
    }
}

ErrorCode ChunkBuilder::build() {
    auto err = store.write_chunk(chunk_num, buff, CHUNK_SIZE_BYTES);
    if (err == ErrorCode::OK) {
        built = true;
    }
    return err;
}

ErrorCode ChunkBuilder::move_pos(usize new_offset) {
    if (new_offset / CHUNK_SIZE_BYTES != chunk_num) {
        auto err = store.write_chunk(chunk_num, buff, CHUNK_SIZE_BYTES);
        if (err != ErrorCode::OK) {
            return err;
        }
        chunk_num = new_offset / CHUNK_SIZE_BYTES;
        if (store.available().count(chunk_num) > 0) {
            uint32_t x;
            err = store.read_chunk(chunk_num, buff, &x);
            if (err != ErrorCode::OK) {
                return err;
            }
        }
    }
    pos = new_offset;
    return ErrorCode::OK;
}
