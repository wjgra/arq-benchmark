#ifndef _ARQ_TRANSMITTER_HPP_
#define _ARQ_TRANSMITTER_HPP_

#include <memory>
#include <span>

#include "arq/conversation_id.hpp"

namespace arq {

constexpr uint16_t maxPacketLength = 1000;

class Packet {
    // add constructor etc.
    uint16_t sequenceNumber;
    uint16_t length;
    std::array<std::byte, maxPacketLength> data;
public:
    void serialise(std::span<std::byte> output) const noexcept;
};

class Transmitter {
public:
    Transmitter(ConversationID id) : id_{id} {

    };
    // Limitation: only one tx session may be in progress at any one time for a given transmitter
    bool send(std::span<const Packet> input); // blocking - returns false if tx in progress
    // bool sendAsync(std::span<const Packet> input); // non-blocking - returns false if tx in progress
    // size_t pending(); // number of packets pending transmission
    // void join(); // blocking
private:
    ConversationID id_;
};

}

#endif


// Usage
/* 
Server:

Transmitter(convID) txer;



txer.send(buffer); // this adds to input buffer

txer.abort(); // this stops 

or

txer.join(); // this blocks

add: txer.status() 

alt: have a loop

while (txer.notDone())
    txer.send()

Client:
Receiver rxer();



 */


/* 

Benchmark:
Txer();
tick();
send(buffer);
tock();

 */