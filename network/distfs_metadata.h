#ifndef DISTFS_DISTFS_METADATA_H
#define DISTFS_DISTFS_METADATA_H


#include <string>
#include "network/proto/distfs.grpc.pb.h"

class DistfsMetadata {
    std::string peer;

    std::shared_ptr<distfs::DistFS::Stub> bootrstrap_peer;

    uint64_t id;
public:
    DistfsMetadata(std::string peer, std::shared_ptr<distfs::DistFS::Stub> bootrstrapPeer, uint64_t id);

    DistfsMetadata();

    const std::string &get_peer() const;

    const std::shared_ptr<distfs::DistFS::Stub> &get_bootrstrap_peer() const;

    uint64_t get_id() const;
};


#endif //DISTFS_DISTFS_METADATA_H