#ifndef _ARQ_TRANSMITTER_HPP_
#define _ARQ_TRANSMITTER_HPP_

#include <concepts>
#include <memory>
#include <thread>
#include <type_traits>

#include "arq/arq_common.hpp"
#include "arq/conversation_id.hpp"
#include "arq/input_buffer.hpp"
#include "arq/rt_buffers/stop_and_wait.hpp"

#include "util/logging.hpp"

namespace arq {

template <typename T>
concept RTBuffer = std::is_base_of<RetransmissionBuffer<T>, T>::value;

template<RTBuffer RTBufferType>
class Transmitter {
public:
    Transmitter(ConversationID id, TransmitFn txFn, ReceiveFn rxFn) : 
        id_{id},
        txFn_{txFn},
        rxFn_{rxFn},
        transmitThread_{[this]() { return this->transmitThread(); }},
        ackThread_{[this]() { return this->ackThread(); }}
    {}

    ~Transmitter() {
        if (transmitThread_.joinable()) {
            transmitThread_.join();
        }
        if (ackThread_.joinable()) {
            ackThread_.join();
        }
    }

    void sendPacket(arq::DataPacket&& packet) {
        inputBuffer_.addPacket(std::forward<arq::DataPacket>(packet));
    }

private:
    void transmitThread() {
        util::logDebug("Transmitter Tx thread started");
        bool receivedEndOfTx = false;
        while (!receivedEndOfTx || retransmissionBuffer_.packetsPending()) {
            auto pkt = retransmissionBuffer_.getPacketData();
            if (pkt.has_value()) {
                DataPacketHeader hdr;
                hdr.deserialise(pkt.value());
                util::logDebug("Retransmitting packet with SN {}", hdr.sequenceNumber_);
                txFn_(pkt.value());
            }
            else if (!receivedEndOfTx && retransmissionBuffer_.readyForNewPacket()) {
                auto nextPkt = inputBuffer_.getPacket();
                if (nextPkt.packet_.isEndOfTx()) { // consider making isEndOfTx a fn of the buffer object
                    util::logDebug("Transmitter received end of EndofTx");
                    receivedEndOfTx = true;
                }
                util::logDebug("Transmitting packet with SN {} and adding to retransmission buffer",
                               nextPkt.info_.sequenceNumber_);
                txFn_(nextPkt.packet_.getReadSpan());
                retransmissionBuffer_.addPacket(std::move(nextPkt));
            }
        }
        util::logDebug("Transmitter Tx thread exited");
    }

    void ackThread() {
        util::logDebug("Transmitter ACK thread started");
        // Temporary implementation
        std::array<std::byte, 1> recvBuffer;
        while(true) {
            auto ret = rxFn_(recvBuffer);
            if (ret.has_value()) {
                util::logInfo("Received ACK for SN {}", std::to_integer<uint8_t>(recvBuffer[0])); // u8 vs u16
                retransmissionBuffer_.acknowledgePacket(std::to_integer<uint8_t>(recvBuffer[0]));
                break;
            }
        }
        util::logDebug("Transmitter ACK thread exited");
    }

    ConversationID id_;
    TransmitFn txFn_;
    ReceiveFn rxFn_;

    // Store packets for transmission that are yet to be transmitted
    InputBuffer inputBuffer_;
    // Store packets that have been transmitted but not acknowledged, and so may require retransmission
    RTBufferType retransmissionBuffer_;
    // 
    std::thread transmitThread_;
    // 
    std::thread ackThread_;
};

}

#endif