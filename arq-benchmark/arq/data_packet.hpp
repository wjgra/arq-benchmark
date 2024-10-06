#ifndef _ARQ_DATA_PACKET_HPP_
#define _ARQ_DATA_PACKET_HPP_

#include <cstddef>
#include <vector>

#include "util/logging.hpp"

namespace arq {

class DataPacket {
public:
    // Maximum permitted size of a DataPacket. Consider whether this should be configurable.
    // Value here chosen such that DataPacket + header fits inside a single Ethernet frame
    static constexpr size_t maxSize = 1400;

    explicit DataPacket(std::vector<std::byte>&& data) : data_{data}
    {
        if (data_.size() > maxSize) {
            util::logWarning("DataPacket of {} bytes truncated to {} bytes",
                             data_.size(),
                             maxSize);
            data_.resize(maxSize);
        }
    };

    DataPacket(const DataPacket& other) = delete;
    DataPacket& operator=(const DataPacket& other) = delete;
    DataPacket(DataPacket&& other) = default;
    DataPacket& operator=(DataPacket&& other) = default;
private:
    std::vector<std::byte> data_;
};

struct DataPacketHeader {
    uint16_t sequenceNumber;
    uint16_t length;
};



}

#endif