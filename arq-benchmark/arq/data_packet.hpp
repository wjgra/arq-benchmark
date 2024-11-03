#ifndef _ARQ_DATA_PACKET_HPP_
#define _ARQ_DATA_PACKET_HPP_

#include <cassert>
#include <cstddef>
#include <exception>
#include <vector>

#include "arq_common.hpp"
#include "arq/conversation_id.hpp"

namespace arq {

constexpr size_t ETH_FRAME_MAX_SIZE = 1500;

struct DataPacketHeader {
    // Identifies the ARQ session
    ConversationID id_;
    // Sequence number assigned by the input buffer
    SequenceNumber sequenceNumber_;
    // Length of the payload
    uint16_t length_; // Q. should len == 0 indicate end of transmission?

    // Serialises the current contents of the DataPacketHeader to the buffer
    bool serialise(std::span<std::byte> buffer) const noexcept;
    // Deserialises the buffer into the DataPacketHeader
    bool deserialise(std::span<const std::byte> buffer) noexcept;

    // Returns the packed size of DataPacketHeader
    static inline constexpr auto size() noexcept {
        return packed_size;
    }
    static inline constexpr size_t packed_size = sizeof(sequenceNumber_) + sizeof(length_) + sizeof(id_);

    bool operator==(const DataPacketHeader& other) const = default;
};

struct DataPacketException : public std::runtime_error {
    explicit DataPacketException(const std::string& what) : std::runtime_error(what) {};
};
 
// Maximum permitted size of a DataPacket. Consider whether this should be configurable.
// Value here chosen such that DataPacket + header fits inside a single Ethernet frame.
constexpr size_t DATA_PKT_MAX_SIZE = ETH_FRAME_MAX_SIZE - DataPacketHeader::size();

class DataPacket {
public:
    // Construct a packet with a default header
    DataPacket();
    // Tx-side: construct a packet with the given header
    DataPacket(const DataPacketHeader& hdr);
    // Rx-side: construct a packet from serialised packet data
    DataPacket(std::span<const std::byte> serialData);
    DataPacket(std::vector<std::byte>&& serialData);

    // Get a copy of the header struct
    DataPacketHeader getHeader() const noexcept;
    // Update the header information and serialise it
    void updateHeader(const DataPacketHeader& hdr);
    // Update header sequence number and serialise it
    void updateSequenceNumber(const SequenceNumber seqNum);

    // Update the length of the payload
    void updateDataLength(const size_t len);

    // Get writable spans of the packet, header or payload
    std::span<std::byte> getSpan() noexcept;
    std::span<std::byte> getHeaderSpan() noexcept;
    std::span<std::byte> getPayloadSpan() noexcept;

    // Get read-only spans of the packet, header or payload
    std::span<const std::byte> getReadSpan() const noexcept;
    std::span<const std::byte> getHeaderReadSpan() const noexcept;
    std::span<const std::byte> getPayloadReadSpan() const noexcept;

    bool operator==(const DataPacket& other) const = default;
private:
    DataPacketHeader header_;
    std::vector<std::byte> data_;

    bool serialiseHeader() noexcept;
    bool deserialiseHeader() noexcept;
};

}

#endif