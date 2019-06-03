#include "distfs_bootstrap.h"

#include "network/proto/distfs.grpc.pb.h"
#include "../common/consts.h"
#include <grpc++/create_channel.h>

using namespace distfs;
using namespace grpc;

DistfsMetadata DistfsBootstrap::fetch_metadata(const std::string &peer) {
    auto channel = CreateChannel(peer, InsecureChannelCredentials());
    if (!channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::milliseconds(10 * PING_TIMEOUT))) {
        throw std::logic_error("Unable to connect to specified peer");
    }

    auto stub = std::shared_ptr<DistFS::Stub>(DistFS::NewStub(channel));
    ClientContext context;
    Empty request;
    Metadata response;
    auto err = stub->GetMetadata(&context, request, &response);
    if (!err.ok()) {
        throw std::logic_error("Unable to fetch metadata");
    }
    std::vector<std::string> hashes;
    for (auto &h : response.block_hash()) {
        hashes.push_back(h);
    }
    return DistfsMetadata(peer, stub, response.fs_id(), std::move(hashes));
}
