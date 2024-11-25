#ifndef _ARQ_RECEIVER_HPP_
#define _ARQ_RECEIVER_HPP_

#include "arq/conversation_id.hpp"

namespace arq {

class Receiver {
public:
    Receiver(ConversationID id) : id_{id} {};

private:
    ConversationID id_;
};

} // namespace arq

#endif