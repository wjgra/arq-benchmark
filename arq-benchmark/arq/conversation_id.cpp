#include <arq/conversation_id.hpp>

arq::ConversationID arq::ConversationIDAllocator::getNewID()
{
    if (allocatedIDs.size() == std::numeric_limits<ConversationID>::max() + 1) {
        throw ConversationIDError();
    }
    // Return next unallocated ID
    while (allocatedIDs.contains(++lastAllocated)) {
    };
    allocatedIDs.insert(lastAllocated);
    return lastAllocated;
}

bool arq::ConversationIDAllocator::registerID(const ConversationID id)
{
    return allocatedIDs.insert(id).second;
}

bool arq::ConversationIDAllocator::releaseID(const ConversationID id)
{
    return allocatedIDs.erase(id);
}
