#ifndef _ARQ_CONVERSATION_ID_HPP_
#define _ARQ_CONVERSATION_ID_HPP_

#include <exception>
#include <unordered_set>

namespace arq {

using ConversationID = uint16_t;

struct ConversationIDError : public std::runtime_error {
    ConversationIDError() : std::runtime_error("No more conversation IDs to allocate") {};
};

class ConversationIDAllocator {
public:
    ConversationID getNewID();
    bool registerID(const ConversationID id);
    bool releaseID(const ConversationID id);
private:
    static ConversationID lastAllocated;
    static std::unordered_set<ConversationID> allocatedIDs;
};

}

#endif