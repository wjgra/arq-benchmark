#include "arq/transmitter.hpp"

arq::Transmitter::Transmitter(ConversationID id) : id_{id} {
    // Create UDP endpoint
};

bool arq::Transmitter::send(std::span<const Packet> input,
                            std::string_view destHost,
                            std::string_view destService) {
(void)input;
(void)destHost;
(void)destService;

// Call virtual functions that get overridden by specific types of ARQ transmitter
return true;
}