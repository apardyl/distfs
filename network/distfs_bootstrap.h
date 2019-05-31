#ifndef DISTFS_DISTFS_BOOTSTRAP_H
#define DISTFS_DISTFS_BOOTSTRAP_H


#include <string>
#include "distfs_metadata.h"

class DistfsBootstrap {
public:
    DistfsMetadata fetch_metadata(const std::string &peer);
};


#endif //DISTFS_DISTFS_BOOTSTRAP_H
