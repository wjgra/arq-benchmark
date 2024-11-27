#ifndef _ARQ_RECEIVER_HPP_
#define _ARQ_RECEIVER_HPP_

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
        endOfTxSeqNum_{std::nullopt} {};

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
        // bool rxEndOfTx = false;
        while (resequencingBuffer_->packetsPending() || endOfTxSeqNum_.has_value() == false) {
            std::array<std::byte, arq::DATA_PKT_MAX_PAYLOAD_SIZE> recvBuffer;
            util::logDebug("Waiting for a data packet");
            auto ret = rxFn_(recvBuffer);
            assert(ret.has_value());

            util::logDebug("Received {} bytes of data", ret.value());

            // arq::ReceiveBufferObject pkt{.packet_{recvBuffer}, .rxTime_ = ClockType::now()};

            arq::DataPacket packet(recvBuffer);
            auto pktHdr = packet.getHeader();
            util::logInfo("Received data packet with length {} and SN {}", pktHdr.length_, pktHdr.sequenceNumber_);

            if (packet.isEndOfTx()) {
                // rxEndOfTx = true;
                endOfTxSeqNum_ = pktHdr.sequenceNumber_;
            }

            // add to rs
            auto ack = resequencingBuffer_->addPacket(std::move(packet));

            // ack
            // auto ack = resequencingBuffer_->getNextAck();
            if (ack.has_value()) {
                util::logInfo("Sending ACK for packet with SN {}", ack.value());
                arq::ControlPacket ctrlPkt = {.sequenceNumber_ = ack.value()};
                std::array<std::byte, sizeof(arq::ControlPacket)> sendBuffer;

                auto ret2 = ctrlPkt.serialise(sendBuffer);
                if (!ret2) {
                    util::logError("Failed to serialise control packet");
                }
                else {
                    txFn_(sendBuffer);
                    util::logDebug("Sent {} bytes", sendBuffer.size());
                }
            }

            // std::array<std::byte, sizeof(arq::SequenceNumber)> ackMsg{};
            // arq::serialiseSeqNum(pktHdr.sequenceNumber_, ackMsg);

            // dataChannel.sendTo(ackMsg, config.common.serverNames.hostName, config.common.serverNames.serviceName);
        }
        util::logInfo("Receiver Rx thread exited");
    }

    // The resequencing thread delivers packets to the output buffer. It continues until
    // an EoT packet has been delivered to the output buffer.
    void resequencingThread()
    {
        while (resequencingBuffer_->packetsPending() || endOfTxSeqNum_.has_value() == false) {
            // auto pkt = resequencingBuffer_->getNextPacket();

            // auto hdr = pkt.getHeader();

            outputBuffer_.addPacket(resequencingBuffer_->getNextPacket()); // output ?
            // if (hdr.sequenceNumber_ == endOfTxSeqNum_) {
            //     util::logInfo("Pushed End of Tx packet (SN {}) to output buffer", hdr.sequenceNumber_);
            // }
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
    //
    std::thread receiveThread_;
    //
    std::thread resequencingThread_;
    // If an EoT has been received, store the SN here
    std::optional<SequenceNumber> endOfTxSeqNum_;
    // If an EoT has been received, store time of last packet reception
};

} // namespace arq

#endif
