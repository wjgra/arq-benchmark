#ifndef _ARQ_RETRANSMISSION_BUFFER_HPP_
#define _ARQ_RETRANSMISSION_BUFFER_HPP_

#include "arq/data_packet.hpp"
#include "arq/tx_buffer.hpp"

namespace arq {

class RetransmissionBuffer {
public:
    void addPacket(TransmitBufferObject&& packet); // Consider a 'buffer' class

    std::optional<std::span<const std::byte>> getRetransmitPacketData(); // also updates last tx time

    void acknowledgePacket(const SequenceNumber seqNum);

    bool packetsPending();
private:
    bool packetAcked = false;
    TransmitBufferObject retransmitPacket_; // In S&W, only one packet is stored for RT
};

}

#endif
