#ifndef _ARQ_RECEIVER_HPP_
#define _ARQ_RECEIVER_HPP_

#include <atomic>
#include <memory>
#include <thread>

#include "arq/arq_common.hpp"
#include "arq/control_packet.hpp"
#include "arq/conversation_id.hpp"
#include "arq/output_buffer.hpp"
#include "arq/resequencing_buffer.hpp"
#include "util/logging.hpp"

namespace arq {

template <typename T>
concept RSBuffer = std::is_base_of<ResequencingBuffer<T>, T>::value;

template <RSBuffer RSBufferType>
class Receiver {
public:
    Receiver(ConversationID id, TransmitFn txFn, ReceiveFn rxFn, std::unique_ptr<RSBufferType>&& rsBuffer_p) :
        id_{id},
        txFn_{txFn},
        rxFn_{rxFn},
        resequencingBuffer_{std::move(rsBuffer_p)},
        receiveThread_{[this]() { return this->receiveThread(); }},
        resequencingThread_{[this]() { return this->resequencingThread(); }},
        pushedEndOfTx_{false} {};

    ~Receiver()
    {
        if (receiveThread_.joinable()) {
            receiveThread_.join();
        }
        if (resequencingThread_.joinable()) {
            resequencingThread_.join();
        }
    }

    // If a packet is available, get the next packet from the output buffer.
    std::optional<ReceiveBufferObject> tryGetPacket() { return outputBuffer_.tryGetPacket(); }

private:
    // The receive thread handles reception of packets from the transmitter. It continues until
    // both an EoT packet has been received and no packets are present in the RS buffer.
    void receiveThread()
    {
        // bool receivedEndOfTx = false; // should really only end when we've pushed EoT
        while (pushedEndOfTx_ == false /* resequencingBuffer_->packetsPending() || !receivedEndOfTx */) {
            std::array<std::byte, MAX_TRANSMISSION_UNIT> recvBuffer;
            util::logDebug("Waiting for a data packet");
            auto bytesRxed = rxFn_(recvBuffer);

            if (!bytesRxed.has_value() || bytesRxed == 0) {
                // No data received
                continue;
            }
            assert(bytesRxed <= MAX_TRANSMISSION_UNIT);

            util::logDebug("Received {} bytes of data", bytesRxed.value());

            arq::DataPacket packet(recvBuffer);
            auto pktHdr = packet.getHeader();
            util::logInfo("Received data packet with length {} and SN {}", pktHdr.length_, pktHdr.sequenceNumber_);

            // if (packet.isEndOfTx()) {
            //     receivedEndOfTx = true;
            // }

            // Add packet to RS and send an ACK if needed
            auto ack = resequencingBuffer_->addPacket(std::move(packet));

            if (ack.has_value()) {
                util::logInfo("Sending ACK for packet with SN {}", ack.value());
                arq::ControlPacket ctrlPkt = {.sequenceNumber_ = ack.value()};
                std::array<std::byte, sizeof(arq::ControlPacket)> sendBuffer;

                if (ctrlPkt.serialise(sendBuffer)) {
                    txFn_(sendBuffer);
                    util::logDebug("Sent {} bytes", sendBuffer.size());
                }
                else {
                    util::logError("Failed to serialise control packet");
                }
            }
        }
        util::logInfo("Receiver Rx thread exited");
    }

    // The resequencing thread delivers packets to the output buffer. It continues until
    // an EoT packet has been delivered to the output buffer.
    void resequencingThread()
    {
        // bool receivedEndOfTx = false;
        while (pushedEndOfTx_ == false /* || resequencingBuffer_->packetsPending() */) {
            auto pkt = resequencingBuffer_->getNextPacket();
            if (pkt.isEndOfTx()) {
                // receivedEndOfTx = true;
                pushedEndOfTx_ = true;
            }
            outputBuffer_.addPacket(std::move(pkt));
        }
        util::logInfo("Receiver resequencing thread exited"); // output thread?
    }

    // Identifies the current conversation (TO DO: issue #24)
    ConversationID id_;
    // Function pointer for raw data transmission
    TransmitFn txFn_;
    // Function pointer for raw data reception
    ReceiveFn rxFn_;
    // Store packets for delivery
    OutputBuffer outputBuffer_;
    // Store packets that have been received but not yet pushed to the output buffer
    std::unique_ptr<RSBufferType> resequencingBuffer_;
    // Thread handling data packet reception
    std::thread receiveThread_;
    // Thread handling pushing data packets to the output buffer
    std::thread resequencingThread_;
    // // If an EoT has been received, store the SN here
    // std::optional<SequenceNumber> endOfTxSeqNum_;
    // If an EoT has been received, store time of last packet reception
    std::atomic<bool> pushedEndOfTx_;
};

} // namespace arq

#endif
