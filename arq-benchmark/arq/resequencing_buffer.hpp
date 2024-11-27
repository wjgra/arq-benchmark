#ifndef _ARQ_RESEQUENCING_BUFFER_HPP_
#define _ARQ_RESEQUENCING_BUFFER_HPP_

#include <chrono>
#include <optional>

#include "arq/rx_buffer_object.hpp"
#include "arq/sequence_number.hpp"

namespace arq {
namespace rs {

// Enforce CRTP requirements using statically checked concepts
// clang-format off
template <typename T>
concept has_addPacket = requires(T t, ReceiveBufferObject&& packet) {
    { t.do_addPacket(std::move(packet)) } -> std::same_as<void>;
};

template <typename T>
concept has_packetsPending = requires(T t) {
    { t.do_packetsPending() } -> std::same_as<bool>;
};

template <typename T>
concept has_getNextPacket = requires(T t) {
    { t.do_getNextPacket() } -> std::same_as<ReceiveBufferObject>;
};

template <typename T>
concept has_getNextAck = requires(T t) {
    { t.do_getNextAck() } -> std::same_as<std::optional<SequenceNumber>>;
};
// clang-format on
} // namespace rs

/*
 * A CRTP interface for an ARQ resequencing buffer (RS). This buffer holds packets
 * that have been received by the receiver but are yet to be forwarded to the output
 * buffer (OB) because they are out of sequence. The various ARQ resequencing schemes are
 * implemented by deriving from this class.
 */
template <typename T>
class ResequencingBuffer {
public:
    ResequencingBuffer()
    {
        static_assert(std::derived_from<T, ResequencingBuffer>);
        static_assert(rs::has_addPacket<T>);
        static_assert(rs::has_packetsPending<T>);
        static_assert(rs::has_getNextPacket<T>);
        static_assert(rs::has_getNextAck<T>);
    }

    // Add a packet to the resequencing buffer
    void addPacket(ReceiveBufferObject&& packet) { static_cast<T*>(this)->do_addPacket(std::move(packet)); }

    // Are there any packets in the resequencing buffer currently?
    bool packetsPending() const noexcept { return static_cast<const T*>(this)->do_packetsPending(); }

    // Retrieve the next packet from the buffer. If no packet is available, block until one is.
    ReceiveBufferObject getNextPacket() { return static_cast<T*>(this)->do_getNextPacket(); }

    // Get the next sequence number to be transmitter as an ACK. If no ack is available, block until one is.
    std::optional<SequenceNumber> getNextAck() { return static_cast<T*>(this)->do_getNextAck(); }
};

} // namespace arq

#endif