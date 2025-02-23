#include "arq/resequencing_buffers/dummy_sctp_rs.hpp"

#include "util/logging.hpp"

arq::rs::DummySCTP::DummySCTP(SequenceNumber firstSeqNum) : nextSequenceNumber_{firstSeqNum} {}

std::optional<arq::SequenceNumber> arq::rs::DummySCTP::do_addPacket(DataPacket&& packet)
{
    auto pktSpan = packet.getReadSpan();
    util::logDebug("Dummy RS buffer received packet of length {} bytes", pktSpan.size());

    if (pktSpan.size() < arq::DataPacketHeader::size()) {
        util::logError("Recieved data is too short to be a data packet");
        return std::nullopt;
    }

    auto receivedPacket = DataPacket(pktSpan);

    if (receivedPacket.isEndOfTx()) {
        util::logDebug("Dummy RS buffer recieved EoT");
        pktSpan = pktSpan.subspan(0, arq::DataPacketHeader::size());
    }
    else {
        /* Unlike UDP, SCTP does not use datagrams, instead delivering a stream of bytes with
         * length equal to the MTU of the interface. Although the data is guaranteed to arrive
         * as in-order packets, we need to trim the extra bytes before passing the packet to
         * the RS buffer.*/
        const size_t packetLen = arq::packet_payload_length + arq::DataPacketHeader::size();
        pktSpan = pktSpan.subspan(0, packetLen);
    }

    auto receivedSequenceNumber = DataPacket(pktSpan).getHeader().sequenceNumber_;
    if (receivedSequenceNumber != nextSequenceNumber_) {
        util::logDebug("Dummy RS buffer rejected packet with SN {}", receivedSequenceNumber);
        return std::nullopt;
    }
    else {
        shadowBuffer_.push(pktSpan);
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
