#ifndef _ARQ_TRANSMITTER_HPP_
#define _ARQ_TRANSMITTER_HPP_

#include <functional>
#include <memory>
#include <span>
#include <thread>

#include "arq/conversation_id.hpp"
#include "arq/input_buffer.hpp"
#include "arq/retransmission_buffer.hpp"

namespace arq {

using TransmitFn = std::function<std::optional<size_t>(std::span<const uint8_t> buffer)>;

class Transmitter {
public:
    Transmitter(ConversationID id, TransmitFn txFn);

    ~Transmitter() {
        transmitThread_.join();
    }

    void sendPacket(arq::DataPacket&& packet) {
        inputBuffer_.addPacket(std::forward<arq::DataPacket>(packet));
    }

private:
    void transmitThread();
    ConversationID id_;
    TransmitFn txFn_;

    // Store packets for transmission that are yet to be transmitted
    InputBuffer inputBuffer_;
    // Store packets that have been transmitted but not acknowledged, and so may require retransmission
    RetransmissionBuffer retransmissionBuffer_;
    // 
    std::thread transmitThread_;
};

}

#endif