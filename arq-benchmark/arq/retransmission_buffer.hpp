#ifndef _ARQ_RETRANSMISSION_BUFFER_HPP_
#define _ARQ_RETRANSMISSION_BUFFER_HPP_

#include "arq/tx_buffer_object.hpp"

namespace arq {
namespace rt {

// Enforce CRTP requirements using statically checked concepts
// clang-format off
template <typename T>
concept has_addPacket = requires(T t, TransmitBufferObject&& packet) {
    { t.do_addPacket(std::move(packet)) } -> std::same_as<void>;
};

template <typename T>
concept has_getPacketData = requires(T t) {
    { t.do_getPacketDataSpan() } -> std::same_as<std::optional<std::span<const std::byte>>>;
};

template <typename T>
concept has_readyForNewPacket = requires(T t) {
    { t.do_readyForNewPacket() } -> std::same_as<bool>;
};

template <typename T>
concept has_packetsPending = requires(T t) {
    { t.do_packetsPending() } -> std::same_as<bool>;
};

template <typename T>
concept has_acknowledgePacket = requires(T t, const SequenceNumber seqNum) {
    { t.do_acknowledgePacket(seqNum) } -> std::same_as<void>;
};
// clang-format on
} // namespace rt

/*
 * A CRTP interface for an ARQ retransmission buffer (RT). This buffer holds
 * packets that have already been transmitted but are yet to be acknowledged
 * by the receiver. The various ARQ retransmission schemes are implemented
 * by deriving from this class.
 */
template <typename T>
class RetransmissionBuffer {
public:
    RetransmissionBuffer(std::chrono::microseconds timeoutInterval) : timeoutInterval_{timeoutInterval}
    {
        static_assert(std::derived_from<T, RetransmissionBuffer>);
        static_assert(rt::has_addPacket<T>);
        static_assert(rt::has_getPacketData<T>);
        static_assert(rt::has_readyForNewPacket<T>);
        static_assert(rt::has_packetsPending<T>);
        static_assert(rt::has_acknowledgePacket<T>);
    }

    // Add a packet to the retransmission buffer
    void addPacket(TransmitBufferObject&& packet) { static_cast<T*>(this)->do_addPacket(std::move(packet)); }

    // Get a span of the next packet for retransmission
    std::optional<std::span<const std::byte>> getPacketDataSpan()
    {
        return static_cast<T*>(this)->do_getPacketDataSpan();
    }

    // Can another packet be added to the retransmission buffer?
    bool readyForNewPacket() const { return static_cast<const T*>(this)->do_readyForNewPacket(); }

    // Are there any packets in the retransmission buffer currently?
    bool packetsPending() const { return static_cast<const T*>(this)->do_packetsPending(); }

    // Update tracking information for a packet which has just been acknowledged
    void acknowledgePacket(const SequenceNumber seqNum) { static_cast<T*>(this)->do_acknowledgePacket(seqNum); }

protected:
    // Has the timeout interval elapsed since this packet was last transmitted?
    bool isPacketTimedOut(const TransmitBufferObject& packet) const
    {
        const auto now = arq::ClockType::now();
        const auto then = packet.info_.lastTxTime_;
        const auto timeSinceLastTx = std::chrono::duration_cast<std::chrono::microseconds>(now - then);
        return timeSinceLastTx.count() > timeoutInterval_.count();
    }
    const std::chrono::microseconds timeoutInterval_;
};

} // namespace arq

#endif
