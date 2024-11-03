#ifndef _ARQ_RETRANSMISSION_BUFFER_HPP_
#define _ARQ_RETRANSMISSION_BUFFER_HPP_

#include "arq/data_packet.hpp"
#include "arq/tx_buffer.hpp"

namespace arq {

class RetransmissionBuffer {
public:
    void addPacket(arq::TransmitBufferObject&& packet); // Consider a 'buffer' class

    std::optional<std::span<std::byte>> getRetransmitPacketData(); // also updates last tx time

    void acknowledgePacket(const SequenceNumber seqNum);

    bool packetsPending();
};

}

#endif
