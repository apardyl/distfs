#include "checksum_engine.h"
#include <openssl/sha.h>

std::string ChecksumEngine::checksum(char *data, uint32_t length) {
    SHA256_CTX sha256_ctx{};
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, data, length);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256_ctx);
    char hex[65];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[64] = 0;
    return std::string(hex);
}
