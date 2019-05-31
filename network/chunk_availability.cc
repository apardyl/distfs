#include "chunk_availability.h"

ChunkAvailability::ChunkAvailability(const std::set<uint32_t> &chunks) : chunks(chunks) {
}

ChunkAvailability::ChunkAvailability(std::set<uint32_t> &&chunks) : chunks(std::move(chunks)) {
}

ChunkAvailability::ChunkAvailability(const std::string &bitmask) {
    for (unsigned i = 0; i < bitmask.length(); i++) {
        char c = bitmask[i];
        for (int j = 0; j < 8; j++) {
            if ((c & (1 << j)) != 0) {
                chunks.insert(i * 8 + j);
            }
        }
    }
}

bool ChunkAvailability::operator[](uint32_t id) const {
    return chunks.count(id) > 0;
}

std::string ChunkAvailability::bitmask() const {
    auto it = chunks.rbegin();
    if (it == chunks.rend()) {
        return "";
    }
    std::string str;
    str.resize((*it / 8) + (*it % 8), '\0');
    for (uint32_t x : chunks) {
        str[x / 8] |= 1 << (x % 8);
    }
    return str;
}

uint32_t ChunkAvailability::count() const {
    return chunks.size();
}

bool ChunkAvailability::operator<(const ChunkAvailability &a) const {
    return count() < a.count();
}

ChunkAvailability::ChunkAvailability() = default;
