#ifndef DISTFS_CHECKSUM_ENGINE_H
#define DISTFS_CHECKSUM_ENGINE_H


#include <cstdint>
#include <string>

class ChecksumEngine {
public:
    std::string checksum(char *data, uint32_t length);
};


#endif //DISTFS_CHECKSUM_ENGINE_H
