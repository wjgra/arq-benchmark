#include "arq/transmitter.hpp"

arq::Transmitter::Transmitter(ConversationID id, TransmitFn txFn) : 
    id_{id},
    txFn_{txFn},
    transmitThread_{[this]() { return this->transmitThread(); }}
{
};

void arq::Transmitter::transmitThread() {

}