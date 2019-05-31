#include <utility>

#include <utility>
#include <random>

#include "distfs_metadata.h"

DistfsMetadata::DistfsMetadata(std::string peer, std::shared_ptr<distfs::DistFS::Stub> bootrstrapPeer, uint64_t id)
        : peer(std::move(peer)), bootrstrap_peer(std::move(bootrstrapPeer)), id(id) {}

const std::string &DistfsMetadata::get_peer() const {
    return peer;
}

const std::shared_ptr<distfs::DistFS::Stub> &DistfsMetadata::get_bootrstrap_peer() const {
    return bootrstrap_peer;
}

uint64_t DistfsMetadata::get_id() const {
    return id;
}

DistfsMetadata::DistfsMetadata() : DistfsMetadata("", std::shared_ptr<distfs::DistFS::Stub>(), 0) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
    id = dis(gen);
}

