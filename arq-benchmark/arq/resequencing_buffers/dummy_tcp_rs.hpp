#ifndef _ARQ_RS_BUFFERS_DUMMY_TCP_HPP_
#define _ARQ_RS_BUFFERS_DUMMY_TCP_HPP_

#include "arq/resequencing_buffer.hpp"

#include <util/safe_queue.hpp>
#include <vector>

namespace arq {
namespace rs {

class DummyTCP : public ResequencingBuffer<DummyTCP> {
public:
    DummyTCP();

    // Standard functions required by ResequencingBuffer CRTP interface
    std::optional<SequenceNumber> do_addPacket(DataPacket&& packet);
    bool do_packetsPending() const noexcept;
    DataPacket do_getNextPacket();

private:
    size_t processReceiveBuffer();
    
    // TCP guarantees that packets are received in order. Store received packets in here to
    // forward onto the OB as requested.
    util::SafeQueue<arq::DataPacket> shadowBuffer_;

    std::vector<std::byte> receiveBuffer_;

    arq::SequenceNumber nextSn_ = arq::firstSequenceNumber;
};

} // namespace rs
} // namespace arq

#endif