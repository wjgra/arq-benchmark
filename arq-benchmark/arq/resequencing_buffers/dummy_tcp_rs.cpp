#include "arq/resequencing_buffers/dummy_tcp_rs.hpp"

#include "util/logging.hpp"

arq::rs::DummyTCP::DummyTCP() {}


// The way to fix this mess is to have a fragmentation function that takes a span.
// The function should push any whole packets (skipping zeroes if needed), and store the remainder in the buffer.
// do_addPacket then appends the current packet to the fragments and calls the frag function.
std::optional<arq::SequenceNumber> arq::rs::DummyTCP::do_addPacket(DataPacket&& packet)
{   
    auto pktSpan = packet.getReadSpan();
    util::logDebug("Received packet of length {} bytes", pktSpan.size());
    for (auto el : pktSpan) {
        std::cout << std::to_integer<int>(el) << " ";

    }

    // Remove trailing zeroes
    size_t lastZeroIdx = pktSpan.size(); // idx after last zero
    while (lastZeroIdx > 0 && pktSpan[lastZeroIdx - 1] == std::byte(0)) {
        --lastZeroIdx;
    }
    auto pktSpanTrimmed = pktSpan.subspan(0, lastZeroIdx);

    // Append to Rx buffer
    receiveBuffer_.insert(receiveBuffer_.end(), pktSpanTrimmed.begin(), pktSpanTrimmed.end());

    // Process fragments
    auto ret = processReceiveBuffer();

    if (ret != 0) {
        util::logDebug("Pushed {} packets to shadow buffer", ret);
    }

    return std::nullopt;


    // // for (auto ch : packet.getReadSpan()) {
    // //     std::cout << std::to_integer<int>(ch) << " ";
    // // }
    // const auto dataPktLen = arq::packet_length + DataPacketHeader::size();

    // const auto hdr = packet.getHeader();

    // if (partialPacket_.size() == 0) { // we're receiving the start of a new packet
    //     assert(hdr.sequenceNumber_ == nextSn_ && hdr.length_ == arq::packet_length);
        
    //     const auto rxedSize = packet.getReadSpan().size();


    //     // assert(rxedSize >= dataPktLen);

    //     // for (size_t i = DataPacketHeader::size() ; i < dataPktLen ; ++i) {
    //     //     assert(packet.getReadSpan()[i] == std::byte(2));
    //     // }
        
    //     assert(rxedSize >= DataPacketHeader::size() );
    //     size_t nextIdx = DataPacketHeader::size();
    //     // for (size_t i = DataPacketHeader::size() ; i < rxedSize && i < dataPktLen; ++i) {

    //     // }
    //     for (; nextIdx < rxedSize ; ++nextIdx) {
    //         if (packet.getReadSpan()[nextIdx] != std::byte(2)) {
    //             break; // reached end of packet or fragment
    //         }
    //     }

    //     if (nextIdx != dataPktLen) { // this is a fragment, not a whole packet
    //         // append fragment to buffer
    //         partialPacket_.insert(partialPacket_.end(), packet.getReadSpan().begin() , packet.getReadSpan().begin() + nextIdx);

    //         return std::nullopt;
    //     }

    //     // bool restEmpty = true;
    //     // for (size_t i = dataPktLen ; i < rxedSize ; ++i) {
    //     //     if (packet.getReadSpan()[i] != std::byte(0)) {
    //     //         restEmpty = false;
    //     //     }
    //     // }
    //     /* size_t */ assert(nextIdx == dataPktLen); // we are at the end of a packet
    //     for (; nextIdx < rxedSize ; ++nextIdx) {
    //         if (packet.getReadSpan()[nextIdx] != std::byte(0)) {
    //             assert(packet.getReadSpan()[nextIdx] == std::byte(1)); // conv ID starts packet
    //             break;
    //         }
    //     }

    //     if (nextIdx == rxedSize) { // rest of frame is empty
    //         shadowBuffer_.push(std::move(packet));
    //         ++nextSn_;
    //     }
    //     else { // there is more in the frame!
    //         // for (size_t i = 0 ; i < rxedSize ; ++i) {
    //         //     std::cout << std::to_integer<int>(packet.getReadSpan()[i]) << " ";
    //         // }
    //         // assert(false);
    //         auto tempPkt1 = DataPacket( packet.getSpan().subspan(0, nextIdx));
    //         shadowBuffer_.push(std::move(tempPkt1));
    //         ++nextSn_;

    //         auto tempPkt = DataPacket( packet.getSpan().subspan(nextIdx));
    //         do_addPacket(std::move(tempPkt));

    //         // for now, assume at most two packets?
    //     }
    //     // size_t i = DataPacketHeader::size();
    //     // for (; i < rxedSize ; ++i) {
    //     //     if (packet.getReadSpan()[i] != std::byte(2)) {
    //     //         break; 
    //     //     }
    //     // }



    //     // if (i == dataPktLen && ( (rxedSize == dataPktLen) || (packet.getReadSpan()[dataPktLen] == std::byte(0))) ) {
    //     //     // we have a whole packet, and nothing else
    //     //     shadowBuffer_.push(std::move(packet));
    //     //     ++nextSn_;
    //     // }
    //     // else {
    //     //     util::logError("WJG: {} ({})", i, dataPktLen);
    //     //     assert(false);
    //     // }
    // }
    // else {
    //     // we are in the middle of a packet - append what we have and process as above
    //     // assert(packet.getReadSpan()[0] == std::byte(2)); // data element
    //     if (packet.getReadSpan()[0] != std::byte(2)) {
    //         assert(packet.getReadSpan()[0] == std::byte(1)); // conv ID
    //         std::cout << "Partial:\n";
    //         for (auto el : partialPacket_) {
    //             std::cout << std::to_integer<int>(el) << " ";

    //         }
    //         std::cout << "Pkt:\n";
    //         for (auto el : packet.getSpan()) {
    //             std::cout << std::to_integer<int>(el) << " ";

    //         }
    //         assert(false);
    //     }
    //     // partialPacket_.insert(partialPacket_.end(), packet.getSpan().begin() , packet.getSpan().end);
    //     for (auto el: packet.getSpan()) {
    //         partialPacket_.push_back(el);
    //     }
        
    //     auto tempPkt = DataPacket(std::span<std::byte>(partialPacket_.begin(), partialPacket_.end()));
    //         do_addPacket(std::move(tempPkt));
        
    //     partialPacket_.resize(0);
    // }
    // return std::nullopt;

    // /* 
    // if no fragment, we're going to receive start of pkt

    // from start of pkt, check SN and len, then count ones
    // if all there, add to buffer
    // if more there, we have another fragment -> push to frag 
    
    // if not all there, fragment not done -> push to frag
    
    // if frag, check SN

    //  */


    // // if start of pkt in last
    // // if (packetFragments_.size() > 0) {
    // //     const auto fragSize = packetFragments_.size();
    // //     std::vector<std::byte> pktData(dataPktLen);
    // //     std::copy(packetFragments_.begin(), packetFragments_.end(), pktData.begin());
    // //     packetFragments.resize(0);

    // //     DataPacket pkt(std::move(pktData));

    // //     assert(pkt.getReadSpan().size() == dataPktLen);
    
    // //     const auto remainder = pkt.getReadSpan().size() - fragSize;


    // // }

    // // if whole pkt in this

    // // if recvd + frag > len
    // // if (packet.getHeader().length_ == arq::packet_length) {
    // //     std::vector<std::byte> pktData();
    // // }

    // // assert(packet.getHeader().length_ == arq::packet_length);

    // // packetFragments_.insert(packetFragments_.end(), packet.getSpan().begin(), packet.getSpan().end());

    // // while (packetFragments_.size() >= dataPktLen) {
    // //     std::vector<std::byte> pktData(packetFragments_.begin(), packetFragments_.begin() + dataPktLen);
    // //     packetFragments_.erase(packetFragments_.begin(), packetFragments_.begin() + dataPktLen);
    // //     DataPacket pkt(std::move(pktData));
    // //     shadowBuffer_.push(std::move(pkt));
    // // }

    // // return std::nullopt;


    // // auto seqNum = packet.getHeader().sequenceNumber_;
    // // shadowBuffer_.push(std::move(packet));
    // // return seqNum;
}


size_t arq::rs::DummyTCP::processReceiveBuffer() {
    const auto dataPktLen = arq::packet_length + DataPacketHeader::size();

    size_t packetsSent = 0;

    while (receiveBuffer_.size() >= dataPktLen) {
        auto temp = DataPacket(std::span<std::byte>(receiveBuffer_.begin(), receiveBuffer_.begin() + dataPktLen));

        // Check packet contents
        auto hdr = temp.getHeader();

        if (hdr.id_ == 1 && hdr.length_ == arq::packet_length && hdr.sequenceNumber_ < nextSn_) {
            // Retransmission of start of packet already pushed.

            size_t startOfNext = DataPacketHeader::size();
            while(receiveBuffer_[++startOfNext] == std::byte(2)) {
            }
            util::logDebug("Dropped {} bytes of retransmitted packet with SN {}", startOfNext, hdr.sequenceNumber_);
            receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin() + startOfNext);
            continue;
        }



        if (hdr.id_ == 1 && hdr.sequenceNumber_ == nextSn_ && hdr.length_ == arq::packet_length) {
            // Packet is correct!
            auto payload = temp.getPayloadReadSpan();
            for (size_t i = 0 ; i < arq::packet_length ; ++i) {
                // assert(payload[i] == std::byte(2));
            }

            // Push to buffer
            util::logDebug("Pushed packet with SN {} to shadow buffer", hdr.sequenceNumber_);
            shadowBuffer_.push(std::move(temp));
            ++nextSn_;
            ++packetsSent;
            receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin() + dataPktLen);

        }
        else {

            // We are half way through a payload retransmission!
            assert(hdr.id_ == 2);
            assert(hdr.sequenceNumber_ == 0x0202);
            assert(hdr.length_ == 0x0202);

            size_t startOfNext = 0;
            while(receiveBuffer_[++startOfNext] == std::byte(2)) {
            }
            util::logDebug("Dropped {} bytes of payload", startOfNext);
            receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin() + startOfNext);

            // for (auto el : receiveBuffer_) {
            //     std::cout << std::to_integer<int>(el) << " ";
            // }
            // assert(false);
        }
    }
    return packetsSent;
}

bool arq::rs::DummyTCP::do_packetsPending() const noexcept
{
    return shadowBuffer_.empty() == false;
}

arq::DataPacket arq::rs::DummyTCP::do_getNextPacket()
{
    return shadowBuffer_.pop_wait();
}
