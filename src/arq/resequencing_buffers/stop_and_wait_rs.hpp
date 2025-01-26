#ifndef _ARQ_RS_BUFFERS_STOP_AND_WAIT_HPP_
#define _ARQ_RS_BUFFERS_STOP_AND_WAIT_HPP_

#include "arq/common/resequencing_buffer.hpp"

#include <condition_variable>
#include <mutex>

namespace arq {
namespace rs {

class StopAndWait : public ResequencingBuffer<StopAndWait> {
public:
    StopAndWait();

    // Standard functions required by ResequencingBuffer CRTP interface
    std::optional<SequenceNumber> do_addPacket(DataPacket&& packet);
    bool do_packetsPending() const noexcept;
    std::optional<DataPacket> do_getNextPacket();

private:
    // In Stop and Wait, only one packet is tracked at a time. If a received packet is not
    // the expected packet, it is ignored.
    SequenceNumber expectedPacketSeqNum_;
    // The packet to deliver to the output buffer.
    std::optional<DataPacket> packetForDelivery_;
};

} // namespace rs
} // namespace arq

#endif