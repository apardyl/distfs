#ifndef DISTFS_COMPRESSIONENGINE_H
#define DISTFS_COMPRESSIONENGINE_H

class CompressionEngine {
public:
    int compress(char *dest, int dest_max_size, char *src, int src_size);

    int decompress(char *dest, int dest_max_size, char *src, int src_size);
};


#endif //DISTFS_COMPRESSIONENGINE_H
