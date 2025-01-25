#include "arq/resequencing_buffers/go_back_n_rs.hpp"

#include "util/logging.hpp"

arq::rs::GoBackN::GoBackN() {}

std::optional<arq::SequenceNumber> arq::rs::GoBackN::do_addPacket(DataPacket&& packet)
{
    auto pkt = packet; // consdier references...
    auto sn = pkt.getHeader().sequenceNumber_;

    std::unique_lock<std::mutex> lock(rsMutex_);

    if (sn == expectedSN_) {
        util::logDebug("Pushed packet with SN {} to shadow buffer", sn);
        if (pkt.isEndOfTx()) {
            endOfTxPushed =
                true; // wjg there shouldn't really be duplication of functionality between here and the receiver...
        }
        ++expectedSN_;
        shadowBuffer_.push(std::move(pkt));
        return sn;
    }
    else {
        // Reject packet
        util::logDebug("Rejected packet with SN {}", sn);
        return std::nullopt;
    }
}

bool arq::rs::GoBackN::do_packetsPending() const noexcept
{
    return !shadowBuffer_.empty();
}

std::optional<arq::DataPacket> arq::rs::GoBackN::do_getNextPacket()
{
    return shadowBuffer_.try_pop();
}
