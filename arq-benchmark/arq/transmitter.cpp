#include "arq/transmitter.hpp"

arq::Transmitter::Transmitter(ConversationID id, TransmitFn txFn, ReceiveFn rxFn) : 
    id_{id},
    txFn_{txFn},
    rxFn_{rxFn},
    transmitThread_{[this]() { return this->transmitThread(); }},
    ackThread_{[this]() { return this->ackThread(); }}
{
}

void arq::Transmitter::transmitThread() {
    // Determine when to pop from input buffer, transmit and retransmit
    // Use a temporary S&W implementation??
    bool receivedEndOfTx = false;
    while (retransmissionBuffer_.packetsPending() || !receivedEndOfTx) {
        if (!receivedEndOfTx) {
            // Get next from IB
            auto next = inputBuffer_.getPacket();
            auto header = next.packet_.getHeader();
            if (header.length_ == 0) {
                util::logInfo("input buffer pushed packet with zero length (SN: {}) - ending transmission of new packets",
                              header.sequenceNumber_);
                receivedEndOfTx = true;
                retransmissionBuffer_.addPacket(std::move(next)); // Temp S&W - push empty packet to RT
            }
            else {
                txFn_(next.packet_.getReadSpan());
                util::logInfo("input buffer pushed packet with length {}, SN: {} - transmitting and adding to retransmission buffer",
                              header.length_,
                              header.sequenceNumber_);
                // Push to retransmission buffer
                retransmissionBuffer_.addPacket(std::move(next));
            }
        }

        // Tx from RT, or get next from IB
        auto retransmitPacket = retransmissionBuffer_.getRetransmitPacketData();
        if (retransmitPacket.has_value()) {
            txFn_(retransmitPacket.value());
            continue; // For temp S&W
        }

    }
}

void arq::Transmitter::ackThread() {
    // Use rxfn to get acks, then signal to RT buffer
    std::array<std::byte, 1> recvBuffer;
    while(true) {
        auto ret = rxFn_(recvBuffer);
        if (ret.has_value()) {
            util::logInfo("Received ACK for SN {}", std::to_integer<uint8_t>(recvBuffer[0])); // u8 vs u16
            retransmissionBuffer_.acknowledgePacket(std::to_integer<uint8_t>(recvBuffer[0]));
            break;
        }
    }
}
