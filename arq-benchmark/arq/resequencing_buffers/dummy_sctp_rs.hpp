#ifndef _ARQ_RS_BUFFERS_DUMMY_SCTP_HPP_
#define _ARQ_RS_BUFFERS_DUMMY_SCTP_HPP_

#include "arq/resequencing_buffer.hpp"

#include <util/safe_queue.hpp>
#include <vector>

namespace arq {
namespace rs {

class DummySCTP : public ResequencingBuffer<DummySCTP> {
public:
    DummySCTP();

    // Standard functions required by ResequencingBuffer CRTP interface
    std::optional<SequenceNumber> do_addPacket(DataPacket&& packet);
    bool do_packetsPending() const noexcept;
    DataPacket do_getNextPacket();

private:
    // SCTP guarantees that packets are received in order. Store received packets in here to
    // forward onto the OB as requested.
    util::SafeQueue<arq::DataPacket> shadowBuffer_;

    // The next sequence number expected by the RS buffer.
    arq::SequenceNumber nextSequenceNumber_ = arq::FIRST_SEQUENCE_NUMBER;
};

} // namespace rs
} // namespace arq

#endif