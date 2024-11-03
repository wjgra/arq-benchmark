#include "arq/transmitter.hpp"

arq::Transmitter::Transmitter(ConversationID id, TransmitFn txFn) : 
    id_{id},
    txFn_{txFn},
    transmitThread_{[this]() { return this->transmitThread(); }},
    ackThread_{[this]() { return this->ackThread(); }}
{
}

void arq::Transmitter::transmitThread() {
    // Determine when to pop from input buffer, transmit and retransmit
    // Use a temporary S&W implementation??
    bool receivedEndOfTx = false;
    while (retransmissionBuffer_.packetsPending() || !receivedEndOfTx) {
        // Tx from RT, or get next from IB
        auto retransmitPacket = retransmissionBuffer_.getRetransmitPacketData();
        if (retransmitPacket.has_value()) {
            txFn_(retransmitPacket.value());
            continue; // For temp S&W
        }

        if (!receivedEndOfTx) {
            // Get next from IB
            auto next = inputBuffer_.getPacket();
            auto header = next.packet_.getHeader();
            if (header.length_ == 0) {
                util::logInfo("input buffer pushed packet with zero length (SN: {}) - ending transmission of new packets",
                              header.sequenceNumber_);
                receivedEndOfTx = true;
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
    }
}

void arq::Transmitter::ackThread() {
    // Use rxfn to get acks, then signal to RT buffer
}
