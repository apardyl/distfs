#include "compression_engine.h"
#include <lz4hc.h>
#include <stdint.h>

int CompressionEngine::compress(char *dest, int dest_max_size, char *src, int src_size) {
    return LZ4_compress_default(src, dest, src_size, dest_max_size);
}

int CompressionEngine::decompress(char *dest, int dest_max_size, char *src, int src_size) {
    return LZ4_decompress_safe(src, dest, src_size, dest_max_size);
}
