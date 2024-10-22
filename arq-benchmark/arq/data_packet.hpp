#ifndef _ARQ_DATA_PACKET_HPP_
#define _ARQ_DATA_PACKET_HPP_

#include <cassert>
#include <cstddef>
#include <cstring>
#include <netdb.h>
#include <vector>

#include "arq/conversation_id.hpp"
#include "util/logging.hpp"

namespace arq {

constexpr size_t ETH_FRAME_MAX_SIZE = 1500;

struct DataPacketHeader {
    ConversationID id_;
    uint16_t sequenceNumber_;
    uint16_t length_;

    bool serialise(std::span<std::byte> buffer) const {
        if (buffer.size() < this->size()) {
            util::logError("Buffer of size {} is too small to serialise a DataPacketHeader of size {}",
                           buffer.size(),
                           this->size());
            return false;
        }

        size_t pos = 0;

        // Serialise conversation ID
        static_assert(sizeof(id_) == 1);
        std::memcpy(buffer.data() + pos, &id_, sizeof(id_));
        pos += sizeof(id_);

        // Serialise SN
        static_assert(sizeof(sequenceNumber_) == 2);
        uint16_t temp = htons(sequenceNumber_);
        std::memcpy(buffer.data() + pos, &temp, sizeof(sequenceNumber_));
        pos += sizeof(sequenceNumber_);

        // Serialise packet length
        static_assert(sizeof(length_) == 2);
        temp = htons(length_);
        std::memcpy(buffer.data() + pos, &temp, sizeof(length_));
        pos += sizeof(length_);

        assert(pos == this->size());
        return true;
    }

    bool deserialise(std::span<std::byte> buffer) {
        if (buffer.size() < this->size()) {
            util::logError("Buffer of size {} is too small to deserialise into a DataPacketHeader of size {}",
                           buffer.size(),
                           this->size());
            return false;
        }

        size_t pos = 0;

        // Deserialise conversation ID
        static_assert(sizeof(id_) == 1);
        id_ = std::to_integer<ConversationID>(buffer[pos]);
        pos += sizeof(id_);

        // Deserialise SN
        static_assert(sizeof(sequenceNumber_) == 2);
        uint16_t temp;
        std::memcpy(buffer.data() + pos, &temp, sizeof(sequenceNumber_));
        sequenceNumber_ = ntohs(temp);
        pos += sizeof(sequenceNumber_);

        // Deserialise packet length
        static_assert(sizeof(length_) == 2);
        std::memcpy(buffer.data() + pos, &temp, sizeof(length_));
        length_ = ntohs(temp);
        pos += sizeof(length_);

        assert(pos == this->size());

        return true;
    }

    constexpr size_t size() const noexcept {
        return sizeof(sequenceNumber_) + sizeof(length_) + sizeof(id_);
    }
};

class DataPacket {
public:
    // Maximum permitted size of a DataPacket. Consider whether this should be configurable.
    // Value here chosen such that DataPacket + header fits inside a single Ethernet frame
    static constexpr size_t maxSize = ETH_FRAME_MAX_SIZE - sizeof(DataPacketHeader);

    explicit DataPacket(std::vector<std::byte>&& data) : data_{data}
    {
        if (data_.size() > maxSize) {
            util::logWarning("DataPacket of {} bytes truncated to {} bytes",
                             data_.size(),
                             maxSize);
            data_.resize(maxSize);
        }
    };

    void serialise() {

    }

private:
    std::vector<std::byte> data_;
};

}

#endif