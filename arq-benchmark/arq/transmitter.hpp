#ifndef _ARQ_TRANSMITTER_HPP_
#define _ARQ_TRANSMITTER_HPP_

#include <memory>
#include <thread>

#include "arq/arq_common.hpp"
#include "arq/conversation_id.hpp"
#include "arq/input_buffer.hpp"
#include "arq/retransmission_buffer.hpp"

#include "util/logging.hpp"

namespace arq {

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
    void ackThread();
    ConversationID id_;
    TransmitFn txFn_;

    // Store packets for transmission that are yet to be transmitted
    InputBuffer inputBuffer_;
    // Store packets that have been transmitted but not acknowledged, and so may require retransmission
    RetransmissionBuffer retransmissionBuffer_;
    // 
    std::thread transmitThread_;
    // 
    std::thread ackThread_;
};

}

#endif