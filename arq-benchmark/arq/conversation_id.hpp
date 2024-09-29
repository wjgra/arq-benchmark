#ifndef _ARQ_CONVERSATION_ID_HPP_
#define _ARQ_CONVERSATION_ID_HPP_

#include <cstdint>
#include <exception>
#include <iostream>
#include <unordered_set>
#include <string_view>

namespace arq {

using ConversationID = uint8_t;

struct ConversationIDError : public std::runtime_error {
    ConversationIDError()  : std::runtime_error("No more conversation IDs to allocate") 
{};
};

class ConversationIDAllocator {
public:
    ConversationID getNewID();
    bool registerID(const ConversationID id);
    bool releaseID(const ConversationID id);
private:
    ConversationID lastAllocated = 0;
    std::unordered_set<ConversationID> allocatedIDs;
};

}

#endif