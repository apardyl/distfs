#include "connection_pool.h"
#include "../common/consts.h"
#include <grpc++/create_channel.h>
#include <random>

using namespace grpc;
using namespace distfs;

ConnectionPool::ConnectionPool(DistfsMetadata &metadata, ChunkStore &store, uint32_t activeConnectionsLimit,
                               uint32_t peerCandidatesLimit, bool active, uint32_t listenPort)
        : active_connections_limit(
        activeConnectionsLimit), peer_candidates(peerCandidatesLimit), metadata(metadata), store(store),
          generator(std::random_device()()), stop_worker(false), listen_port(listenPort) {
    std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
    node_id = dis(generator);
    if (!metadata.get_peer().empty() && metadata.get_bootrstrap_peer()) {
        Connection connection;
        connection.stub = metadata.get_bootrstrap_peer();
        connection.url = metadata.get_peer();
        connections.push_back(std::move(connection));
        active_peers.insert(metadata.get_peer());
    }
    if (active) {
        worker();
        worker_thread = std::thread(([this]() -> void {
            while (!stop_worker.load()) {
                this->worker();
            }
        }));
    }
}

ErrorCode ConnectionPool::fetch_chunk(uint32_t id) {
    ChunkPartResponse response;
    auto buff = std::unique_ptr<char>(new char[CHUNK_SIZE_BYTES]);
    ClientContext context;
    ChunkRequest request;
    request.set_id(id);

    while (true) {
        auto[connection, code] = connection_for_block(id);
        if (code != ErrorCode::OK) {
            return code;
        }

        uint32_t offset = 0;
        auto reader = connection->GetChunk(&context, request);
        while (reader->Read(&response)) {
            memcpy(buff.get() + offset, response.data().c_str(), response.data().size());
            offset += response.data().size();
        }

        auto status = reader->Finish();
        if (status.ok()) {
            std::string checksum = checksumEngine.checksum(buff.get(), offset);
            if (checksum != metadata.get_hashes()[id]) {
                debug_print("Chunk hash mismatch for %d\n", id);
                continue;
            }
            store.write_chunk(id, buff.get(), offset);
            debug_print("Chunk downloaded %d\n", id);
            return ErrorCode::OK;
        }
    }
}

std::tuple<std::shared_ptr<distfs::DistFS::Stub>, ErrorCode> ConnectionPool::connection_for_block(uint32_t id) {
    std::lock_guard<std::mutex> guard(connections_mutex);
    std::vector<std::shared_ptr<distfs::DistFS::Stub>> a;
    for (auto &c : connections) {
        if (c.availability[id]) {
            a.push_back(c.stub);
        }
    }
    if (!a.empty()) {
        std::uniform_int_distribution<uint64_t> dis(0, a.size() - 1);
        return std::make_tuple(a[dis(generator)], ErrorCode::OK);
    }

    debug_print("No peer for chunk %d\n", id);
    return std::make_tuple(std::shared_ptr<distfs::DistFS::Stub>(), ErrorCode::NOT_FOUND);
}

void ConnectionPool::worker_get_info() {
    time_t now;
    time(&now);
    std::lock_guard<std::mutex> guard(connections_mutex);

    Info request;
    request.set_fs_id(metadata.get_id());
    request.set_node_id(node_id);
    request.set_listen_port(listen_port);
    request.set_available(ChunkAvailability(store.available()).bitmask());

    for (auto it = connections.begin(); it != connections.end();) {
        auto &c = *it;
        if (now - c.last_info > INFO_REFRESH_S) {
            ClientContext context;
            Info response;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT));
            auto err = c.stub->InfoExchange(&context, request, &response);
            if (err.ok()) {
                c.availability = ChunkAvailability(response.available());
                time(&c.last_info);
                it++;
                debug_print("Info exchange with %s\n", c.url.c_str());
            } else {
                debug_print("No info exchange response from %s\n", c.url.c_str());
                active_peers.erase(c.url);
                connections.erase(it++);
            }
        } else {
            it++;
        }
    }
    connections.sort();
}

void ConnectionPool::worker_get_pex() {
    time_t now;
    time(&now);
    std::lock_guard<std::mutex> guard(connections_mutex);
    std::lock_guard<std::mutex> guard2(peer_list_mutex);

    PeerList request;
    for (auto &connection : connections) {
        request.add_peer(connection.url);
    }

    for (auto it = connections.begin(); it != connections.end();) {
        auto &c = *it;
        if (now - c.last_pex > PEX_REFRESH_S) {
            ClientContext context;
            PeerList response;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT));
            auto err = c.stub->PeerExchange(&context, request, &response);
            if (err.ok()) {
                for (auto &p : response.peer()) {
                    if (active_peers.count(p) == 0) {
                        peer_candidates.push(p);
                    }
                }
                time(&c.last_pex);
                it++;
                debug_print("Peer exchange with %s\n", c.url.c_str());
            } else {
                debug_print("No peer exchange response from %s\n", c.url.c_str());
                active_peers.erase(c.url);
                connections.erase(it++);
            }
        } else {
            it++;
        }
    }
    connections.sort();
}

void ConnectionPool::worker_expand() {
    std::lock_guard<std::mutex> guard(peer_list_mutex);
    for (int i = 0; i < 5 && active_peers.size() < active_connections_limit && peer_candidates.size() > 0; i++) {
        auto p = peer_candidates.pop();
        debug_print("Attempting connection to %s\n", p.c_str());
        auto channel = CreateChannel(p, InsecureChannelCredentials());
        if (!channel->WaitForConnected(
                std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT))) {
            debug_print("Timed out connection attempt to %s\n", p.c_str());
            continue;
        }
        auto stub = std::shared_ptr<DistFS::Stub>(DistFS::NewStub(channel));
        Info request;
        request.set_fs_id(metadata.get_id());
        request.set_node_id(node_id);
        request.set_listen_port(listen_port);
        request.set_available(ChunkAvailability(store.available()).bitmask());
        ClientContext context;
        Info response;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT));
        auto err = stub->InfoExchange(&context, request, &response);
        if (!err.ok() || response.fs_id() != metadata.get_id()) {
            debug_print("Invalid response for connection attempt to %s\n", p.c_str());
            continue;
        }
        if (response.node_id() == node_id) {
            debug_print("Ignoring loop %s\n", p.c_str());
        }
        debug_print("Response from potential peer %s\n", p.c_str());

        Connection connection;
        connection.stub = stub;
        connection.availability = ChunkAvailability(response.available());
        time(&connection.last_info);
        {
            std::lock_guard<std::mutex> guard2(connections_mutex);
            connections.push_back(std::move(connection));
            connections.sort();
        }
        active_peers.insert(p);
        debug_print("Connected to %s\n", p.c_str());
    }

}

void ConnectionPool::worker_explore() {
    if (time(nullptr) - last_explore > 15) {
        uint32_t min_chunks = 0;
        {
            std::lock_guard<std::mutex> guard(connections_mutex);
            if (!connections.empty()) {
                min_chunks = (*connections.begin()).availability.count();
            }
        }

        std::lock_guard<std::mutex> guard(peer_list_mutex);

        int i = 0;

        while (peer_candidates.size() > 0 && i++ < 10) {
            auto p = peer_candidates.pop();
            debug_print("Attempting connection to (explore) %s\n", p.c_str());
            auto channel = CreateChannel(p, InsecureChannelCredentials());
            if (!channel->WaitForConnected(
                    std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT))) {
                debug_print("Timed out connection attempt to %s\n", p.c_str());
                continue;
            }
            auto stub = std::shared_ptr<DistFS::Stub>(DistFS::NewStub(channel));
            Info request;
            request.set_fs_id(metadata.get_id());
            request.set_node_id(node_id);
            request.set_listen_port(listen_port);
            request.set_available(ChunkAvailability(store.available()).bitmask());
            ClientContext context;
            Info response;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT));
            auto err = stub->InfoExchange(&context, request, &response);
            if (!err.ok() || response.fs_id() != metadata.get_id()) {
                debug_print("Invalid response for connection attempt to %s\n", p.c_str());
                continue;
            }
            if (response.node_id() == node_id) {
                debug_print("Ignoring loop %s\n", p.c_str());
            }
            if (ChunkAvailability(response.available()).count() <= min_chunks) {
                debug_print("Peer not worth connecting %s\n", p.c_str());
                continue;
            }
            debug_print("Response from potential peer %s\n", p.c_str());

            Connection connection;
            connection.stub = stub;
            connection.availability = ChunkAvailability(response.available());
            time(&connection.last_info);
            {
                std::lock_guard<std::mutex> guard2(connections_mutex);
                auto first = connections.begin();
                debug_print("Replacing %s with %s\n", first->url.c_str(), p.c_str());
                active_peers.erase(first->url);
                connections.erase(first);
                connections.push_back(std::move(connection));
                connections.sort();
            }
            active_peers.insert(p);
        }

    }
}

void ConnectionPool::worker() {
    worker_get_info();

    worker_get_pex();

    worker_expand();

    worker_explore();

    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void ConnectionPool::info_from(const std::string &peer, const ChunkAvailability &chunkAvailability) {
    {
        std::lock_guard<std::mutex> guard(peer_list_mutex);
        if (active_peers.count(peer) == 0) {
            if (peer_candidates.push(peer)) {
                debug_print("New peer candidate %s\n", peer.c_str());
            }
        }
    }
    {
        std::lock_guard<std::mutex> guard(connections_mutex);
        bool changed = false;
        for (Connection &c : connections) {
            if (c.url == peer) {
                c.availability = chunkAvailability;
                debug_print("Received info from %s\n", peer.c_str());
                changed = true;
                break;
            }
        }
        if (changed) {
            connections.sort();
        }
    }
}

ConnectionPool::~ConnectionPool() {
    stop_worker.store(true);
    worker_thread.join();
}

uint64_t ConnectionPool::get_node_id() {
    return node_id;
}

uint64_t ConnectionPool::get_fs_id() {
    return metadata.get_id();
}

const std::vector<string> &ConnectionPool::get_block_hashes() {
    return metadata.get_hashes();
}

uint32_t ConnectionPool::get_listen_port() {
    return listen_port;
}

bool ConnectionPool::Connection::operator<(const ConnectionPool::Connection &b) const {
    return availability < b.availability;
}
