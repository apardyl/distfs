#include <utility>

#include <utility>

#include <utility>
#include <random>

#include "distfs_metadata.h"
#include "../common/checksum_engine.h"
#include "../common/consts.h"

DistfsMetadata::DistfsMetadata(std::string peer, std::shared_ptr<distfs::DistFS::Stub> bootrstrapPeer, uint64_t id,
                               std::vector<std::string> blockHashes)
        : peer(std::move(peer)), bootrstrap_peer(std::move(bootrstrapPeer)), id(id),
          block_hashes(std::move(blockHashes)) {}

const std::string &DistfsMetadata::get_peer() const {
    return peer;
}

const std::shared_ptr<distfs::DistFS::Stub> &DistfsMetadata::get_bootrstrap_peer() const {
    return bootrstrap_peer;
}

uint64_t DistfsMetadata::get_id() const {
    return id;
}

DistfsMetadata::DistfsMetadata(ChunkStore &store) {
    ChecksumEngine checksumEngine;
    std::unique_ptr buffer = std::unique_ptr<char>(new char[CHUNK_SIZE_BYTES]);

    uint32_t max_id = store.available().size();
    block_hashes.resize(max_id);

    for (uint32_t i = 0; i < max_id; i++) {
        uint32_t size;
        ErrorCode err = store.read_chunk(i, buffer.get(), &size);
        if (err != ErrorCode::OK) {
            throw std::logic_error("Store missing data block " + std::to_string(i));
        }
        block_hashes[i] = checksumEngine.checksum(buffer.get(), size);
    }

    id = 1;
    for (auto &s : block_hashes) {
        for (auto c : s) {
            id *= c;
        }
    }
}

const std::vector<std::string> &DistfsMetadata::get_hashes() const {
    return block_hashes;
}

