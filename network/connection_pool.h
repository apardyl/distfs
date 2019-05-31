#ifndef DISTFS_CONNECTION_POOL_H
#define DISTFS_CONNECTION_POOL_H


#include <cstdint>
#include <thread>
#include "../data/chunk_external_provider.h"
#include "distfs_metadata.h"
#include "network/proto/distfs.grpc.pb.h"
#include "../common/ordered_set.h"
#include "chunk_availability.h"

class ConnectionPool : public ChunkExternalProvider {
    uint32_t active_connections_limit;

    std::mutex peer_list_mutex;
    OrderedSet peer_candidates;
    std::set<std::string> active_peers;

    DistfsMetadata &metadata;
    ChunkStore &store;

    class Connection {
    public:
        std::shared_ptr<distfs::DistFS::Stub> stub;
        ChunkAvailability availability;
        std::string url;
        time_t last_info;
        time_t last_pex;

        bool operator<(const Connection &b) const;
    };

    std::list<Connection> connections;
    std::mutex connections_mutex;

    std::tuple<std::shared_ptr<distfs::DistFS::Stub>, ErrorCode> connection_for_block(uint32_t id);

    std::thread worker_thread;
    std::atomic<bool> stop_worker;

    time_t last_explore = 0;

    uint64_t node_id;

    void worker();

    void worker_get_info();

    void worker_get_pex();

    void worker_expand();

    void worker_explore();

public:
    explicit ConnectionPool(DistfsMetadata &metadata, ChunkStore &store, uint32_t activeConnectionsLimit = 20,
                            uint32_t peerCandidatesLimit = 200, bool active = true);

    ~ConnectionPool();

    ErrorCode fetch_chunk(uint32_t id) override;

    void connection_from(const std::string &peer);

    void info_from(const std::string &peer, const ChunkAvailability &chunkAvailability);

    template<typename T, typename Y>
    void pex_from(const std::string &peer, const T &incoming_list, Y &outgoing_list) {
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
                outgoing_list.add_peer(connection.url);
                if (connection.url == peer) {
                    time(&connection.last_pex);
                }
            }
        }
    }

    uint64_t get_node_id();

    uint64_t get_fs_id();
};


#endif //DISTFS_CONNECTION_POOL_H
