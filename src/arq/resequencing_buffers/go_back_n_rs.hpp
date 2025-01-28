#ifndef _ARQ_RS_BUFFERS_GO_BACK_N_HPP_
#define _ARQ_RS_BUFFERS_GO_BACK_N_HPP_

#include "arq/common/resequencing_buffer.hpp"
#include "util/safe_queue.hpp"

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace arq {
namespace rs {

class GoBackN : public ResequencingBuffer<GoBackN> {
public:
    GoBackN(SequenceNumber firstSeqNum = FIRST_SEQUENCE_NUMBER);

    // Standard functions required by ResequencingBuffer CRTP interface
    std::optional<SequenceNumber> do_addPacket(DataPacket&& packet);
    bool do_packetsPending() const noexcept;
    std::optional<DataPacket> do_getNextPacket();

private:
    // In Go Back N, only one packet is tracked at a time on the recieve side.
    // If a received packet is not the expected packet, it is ignored.
    // // Has an EoT packet been pushed to the OB?
    bool endOfTxPushed = false;
    util::SafeQueue<arq::DataPacket> shadowBuffer_;

    // The next sequence number expected by the RS buffer.
    SequenceNumber nextSequenceNumber_;
};

} // namespace rs
} // namespace arq

#endif