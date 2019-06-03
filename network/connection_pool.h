#ifndef DISTFS_CONNECTION_POOL_H
#define DISTFS_CONNECTION_POOL_H


#include <cstdint>
#include <thread>
#include <random>
#include "../data/chunk_external_provider.h"
#include "distfs_metadata.h"
#include "network/proto/distfs.grpc.pb.h"
#include "../common/ordered_set.h"
#include "../common/checksum_engine.h"
#include "chunk_availability.h"
#include "../common/consts.h"

class ConnectionPool : public ChunkExternalProvider {
    uint32_t active_connections_limit;

    std::mutex peer_list_mutex;
    OrderedSet peer_candidates;
    std::set<std::string> active_peers;

    DistfsMetadata &metadata;
    ChunkStore &store;

    std::mt19937 generator;
    ChecksumEngine checksumEngine;

    class Connection {
    public:
        std::shared_ptr<distfs::DistFS::Stub> stub;
        ChunkAvailability availability;
        std::string url;
        uint64_t n_id;
        time_t last_info = 0;
        time_t last_pex = 0;

        bool operator<(const Connection &b) const;
    };

    std::list<Connection> connections;
    std::mutex connections_mutex;

    std::tuple<std::shared_ptr<distfs::DistFS::Stub>, ErrorCode> connection_for_block(uint32_t id);

    std::thread worker_thread;
    std::atomic<bool> stop_worker;

    time_t last_explore = 0;

    uint64_t node_id;

    uint32_t listen_port;

    void worker();

    void worker_get_info();

    void worker_get_pex();

    void worker_expand();

    void worker_explore();

public:
    explicit ConnectionPool(DistfsMetadata &metadata, ChunkStore &store, uint32_t activeConnectionsLimit,
                            uint32_t peerCandidatesLimit, bool active, uint32_t listenPort);

    ~ConnectionPool();

    ErrorCode fetch_chunk(uint32_t id) override;

    void info_from(const std::string &peer, uint64_t n_id, const ChunkAvailability &chunkAvailability);

    template<typename T, typename Y>
    void pex_from(const std::string &peer, uint64_t n_id, const T &incoming_list, Y &outgoing_list) {
        {
            std::lock_guard<std::mutex> guard(peer_list_mutex);
            for (const std::string &p : incoming_list) {
                if (active_peers.count(p) == 0) {
                    peer_candidates.push(p);
                }
            }
        }
        {
            for (auto &connection : connections) {
                if (connection.n_id == n_id) {
                    time(&connection.last_pex);
                } else {
                    outgoing_list.add_peer(connection.url);
                }
            }
        }
    }

    uint64_t get_node_id();

    uint64_t get_fs_id();

    uint32_t get_listen_port();

    const std::vector<std::string> &get_block_hashes();
};


#endif //DISTFS_CONNECTION_POOL_H
