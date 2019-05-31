#include "connection_pool.h"
#include "../common/consts.h"
#include <grpc++/create_channel.h>
#include <random>

using namespace grpc;
using namespace distfs;

ConnectionPool::ConnectionPool(DistfsMetadata &metadata, ChunkStore &store, uint32_t activeConnectionsLimit,
                               uint32_t peerCandidatesLimit, bool active)
        : active_connections_limit(
        activeConnectionsLimit), peer_candidates(peerCandidatesLimit), metadata(metadata), store(store),
          stop_worker(false) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
    node_id = dis(gen);
    if (!metadata.get_peer().empty() && metadata.get_bootrstrap_peer()) {
        Connection connection;
        connection.stub = metadata.get_bootrstrap_peer();
        Info request;
        request.set_fs_id(metadata.get_id());
        request.set_available(ChunkAvailability(store.available()).bitmask());
        ClientContext context;
        Info response;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT));
        auto err = connection.stub->InfoExchange(&context, request, &response);
        if (!err.ok() || response.fs_id() != metadata.get_id() || response.node_id() == node_id) {
            throw std::logic_error("Lost connection during init");
        }
        connection.availability = ChunkAvailability(response.available());
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
            store.write_chunk(id, buff.get(), offset);
            return ErrorCode::OK;
        }
    }
}

std::tuple<std::shared_ptr<distfs::DistFS::Stub>, ErrorCode> ConnectionPool::connection_for_block(uint32_t id) {
    std::lock_guard<std::mutex> guard(connections_mutex);
    for (auto &c : connections) {
        if (c.availability[id]) {
            return std::make_tuple(c.stub, ErrorCode::OK);
        }
    }
    return std::make_tuple(std::shared_ptr<distfs::DistFS::Stub>(), ErrorCode::NOT_FOUND);
}

void ConnectionPool::worker_get_info() {
    time_t now;
    time(&now);
    std::lock_guard<std::mutex> guard(connections_mutex);

    Info request;
    request.set_fs_id(metadata.get_id());
    request.set_node_id(node_id);
    request.set_available(ChunkAvailability(store.available()).bitmask());

    for (auto it = connections.begin(); it != connections.end();) {
        auto &c = *it;
        if (now - c.last_info > INFO_REFRESH_S) {
            ClientContext context;
            Info response;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT));
            auto err = c.stub->InfoExchange(&context, request, &response);
            if (err.ok()) {
                c.availability = ChunkAvailability(request.available());
                time(&c.last_info);
                it++;
            } else {
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
            } else {
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
        auto channel = CreateChannel(p, InsecureChannelCredentials());
        if (!channel->WaitForConnected(
                std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT))) {
            continue;
        }
        auto stub = std::shared_ptr<DistFS::Stub>(DistFS::NewStub(channel));
        Info request;
        request.set_fs_id(metadata.get_id());
        request.set_node_id(node_id);
        request.set_available(ChunkAvailability(store.available()).bitmask());
        ClientContext context;
        Info response;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT));
        auto err = stub->InfoExchange(&context, request, &response);
        if (!err.ok() || response.fs_id() != metadata.get_id() || response.node_id() == node_id) {
            continue;
        }

        Connection connection;
        connection.stub = stub;
        connection.availability = ChunkAvailability(response.available());
        time(&connection.last_info);
        {
            std::lock_guard<std::mutex> guard2(connections_mutex);
            active_peers.erase((*connections.begin()).url);
            connections.erase(connections.begin());
            connections.push_back(std::move(connection));
            connections.sort();
        }
        active_peers.insert(p);
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
            auto channel = CreateChannel(p, InsecureChannelCredentials());
            if (!channel->WaitForConnected(
                    std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT))) {
                continue;
            }
            auto stub = std::shared_ptr<DistFS::Stub>(DistFS::NewStub(channel));
            Info request;
            request.set_fs_id(metadata.get_id());
            request.set_available(ChunkAvailability(store.available()).bitmask());
            ClientContext context;
            Info response;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PING_TIMEOUT));
            auto err = stub->InfoExchange(&context, request, &response);
            if (!err.ok() || response.fs_id() != metadata.get_id() || response.node_id() == node_id ||
                ChunkAvailability(response.available()).count() <= min_chunks) {
                continue;
            }

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

void ConnectionPool::connection_from(const std::string &peer) {
    std::lock_guard<std::mutex> guard(peer_list_mutex);
    if (active_peers.count(peer) == 0) {
        peer_candidates.push(peer);
    }
}

void ConnectionPool::info_from(const std::string &peer, const ChunkAvailability &chunkAvailability) {
    std::lock_guard<std::mutex> guard(connections_mutex);
    bool changed = false;
    for (Connection &c : connections) {
        if (c.url == peer) {
            c.availability = chunkAvailability;
            changed = true;
            break;
        }
    }
    if (changed) {
        connections.sort();
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

bool ConnectionPool::Connection::operator<(const ConnectionPool::Connection &b) const {
    return availability < b.availability;
}
