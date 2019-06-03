#ifndef DISTFS_CONSTS_H
#define DISTFS_CONSTS_H

#define CHUNK_SIZE_BYTES (1024 * 1024 * 4) // 4 MB

#define WIRE_CHUNK_SIZE_BYTES (1024 * 410) // 0.4 MB

#define PING_TIMEOUT 1000

#define INFO_REFRESH_S 10
#define PEX_REFRESH_S 60

typedef uint64_t usize;

#define DEBUG 0
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#endif //DISTFS_CONSTS_H
