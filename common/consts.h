#ifndef DISTFS_CONSTS_H
#define DISTFS_CONSTS_H

#define CHUNK_SIZE_BYTES (1024 * 1024 * 4) // 4 MB

#define WIRE_CHUNK_SIZE_BYTES (1024 * 410) // 0.4 MB

#define PING_TIMEOUT 200

#define INFO_REFRESH_S 10
#define PEX_REFRESH_S 60

typedef uint64_t usize;

#endif //DISTFS_CONSTS_H
