#ifndef _ARQ_RECEIVER_HPP_
#define _ARQ_RECEIVER_HPP_

#include <memory>
#include <thread>

#include "arq/conversation_id.hpp"
#include "arq/output_buffer.hpp"
#include "arq/resequencing_buffer.hpp"

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
        resequencingThread_{[this]() { return this->resequencingThread(); }} {};

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
    std::optional<ReceiveBufferObject> tryGetPacket();

private:
    void receiveThread() {}

    void resequencingThread() {}

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
};

} // namespace arq

#endif
