#include "arq/resequencing_buffers/dummy_sctp_rs.hpp"

#include "util/logging.hpp"

arq::rs::DummySCTP::DummySCTP() {}

std::optional<arq::SequenceNumber> arq::rs::DummySCTP::do_addPacket(DataPacket&& packet)
{
    auto pktSpan = packet.getReadSpan();
    util::logDebug("Dummy RS buffer received packet of length {} bytes", pktSpan.size());

    /* Unlike UDP, SCTP does not use datagrams, instead delivering a stream of bytes with
     * length equal to the MTU of the interface. Although the data is guaranteed to arrive
     * as in-order packets, trailing zeroes are included. Assert that these are present
     * then trim them before passing to the shadow buffer.*/
    const size_t packetLen = arq::packet_payload_length + arq::DataPacketHeader::size();
    for (size_t i = packetLen; i < MAX_TRANSMISSION_UNIT; ++i) {
        assert(std::to_integer<int>(pktSpan[i]) == 0);
    }

    auto pktSpanTrimmed = pktSpan.subspan(0, DATA_PKT_MAX_PAYLOAD_SIZE);

    auto receivedSequenceNumber = DataPacket(pktSpanTrimmed).getHeader().sequenceNumber_;
    if (receivedSequenceNumber != nextSequenceNumber_) {
        util::logDebug("Dummy RS buffer rejected packet with SN {}", receivedSequenceNumber);
        return std::nullopt;
    }
    else {
        shadowBuffer_.push(pktSpanTrimmed);
        util::logDebug("Dummy RS buffer pushed packet with SN {} to shadow buffer", receivedSequenceNumber);
        ++nextSequenceNumber_;
        return receivedSequenceNumber;
    }
}

bool arq::rs::DummySCTP::do_packetsPending() const noexcept
{
    return !shadowBuffer_.empty();
}

std::optional<arq::DataPacket> arq::rs::DummySCTP::do_getNextPacket()
{
    return shadowBuffer_.try_pop();
}
