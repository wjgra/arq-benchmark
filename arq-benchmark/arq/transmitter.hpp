#ifndef _ARQ_TRANSMITTER_HPP_
#define _ARQ_TRANSMITTER_HPP_

#include <memory>
#include <span>

#include "arq/conversation_id.hpp"

namespace arq {

class Packet {
    static constexpr uint16_t maxPacketLength = 500;
    // add constructor etc.
    uint16_t sequenceNumber;
    uint16_t length;
    std::array<std::byte, maxPacketLength> data;
public:
    void serialise(std::span<std::byte> output) const noexcept;
};

// Idea: InputBuffer class that you can feed indefinitely.
// When you want the transmitter to stop, you feed it a special packet.
// Then just call send(InputBuffer) on transmitter.

// Q: do we fragment packets for the purposes of ARQ?

class Transmitter {
public:
    Transmitter(ConversationID id);
    Transmitter() = delete;
    // Blocking send function. Returns true if transmission succeeds, false otherwise.
    bool send(std::span<const Packet> input,
              std::string_view destHost,
              std::string_view destService);
    // To implement:
    // Non-blocking send function - only one session may be in progress at once? Alt: have a queue of tx jobs
    // bool sendAsync(std::span<const Packet> input); // non-blocking - returns false if tx in progress
    // size_t pending(); // number of packets pending transmission
    // void join(); // blocking
private:
    ConversationID id_;
};

}

#endif