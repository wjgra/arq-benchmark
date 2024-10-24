#include "arq/data_packet.hpp"

#include <cstring>
#include <netdb.h>

#include "util/logging.hpp"

bool arq::DataPacketHeader::serialise(std::span<std::byte> buffer) const noexcept
{
    if (buffer.size() < this->size()) {
        util::logError("Buffer of size {} is too small to serialise a DataPacketHeader of size {}",
                        buffer.size(),
                        this->size());
        return false;
    }

    size_t pos = 0;

    // Serialise conversation ID
    static_assert(sizeof(id_) == 1);
    std::memcpy(buffer.data() + pos, &id_, sizeof(id_));
    pos += sizeof(id_);

    // Serialise SN
    static_assert(sizeof(sequenceNumber_) == 2);
    uint16_t temp = htons(sequenceNumber_);
    std::memcpy(buffer.data() + pos, &temp, sizeof(sequenceNumber_));
    pos += sizeof(sequenceNumber_);

    // Serialise packet length
    static_assert(sizeof(length_) == 2);
    temp = htons(length_);
    std::memcpy(buffer.data() + pos, &temp, sizeof(length_));
    pos += sizeof(length_);

    assert(pos == this->size());
    return true;
}

bool arq::DataPacketHeader::deserialise(std::span<const std::byte> buffer) noexcept
{
    if (buffer.size() < this->size()) {
        util::logError("Buffer of size {} is too small to deserialise into a DataPacketHeader of size {}",
                        buffer.size(),
                        this->size());
        return false;
    }

    size_t pos = 0;

    // Deserialise conversation ID
    static_assert(sizeof(id_) == 1);
    id_ = std::to_integer<decltype(id_)>(buffer[pos]);
    pos += sizeof(id_);

    // Deserialise SN
    static_assert(sizeof(sequenceNumber_) == 2);
    uint16_t temp;
    std::memcpy(&temp, buffer.data() + pos, sizeof(sequenceNumber_));
    sequenceNumber_ = ntohs(temp);
    pos += sizeof(sequenceNumber_);

    // Deserialise packet length
    static_assert(sizeof(length_) == 2);
    std::memcpy(&temp, buffer.data() + pos, sizeof(length_));
    length_ = ntohs(temp);
    pos += sizeof(length_);

    assert(pos == this->size());

    return true;
}

arq::DataPacket::DataPacket(const DataPacketHeader& hdr)
{
    setHeader(hdr);
}

arq::DataPacket::DataPacket(std::span<const std::byte> serialData)
{
    data_.assign(serialData.begin(), serialData.end());
    if (!deserialiseHeader()) {
        throw DataPacketException("serialData is not long enough to contain header");
    }
}

arq::DataPacket::DataPacket(std::vector<std::byte>&& serialData) : 
    data_{std::move(serialData)}
{
    if (!deserialiseHeader()) {
        throw DataPacketException("serialData is not long enough to contain header");
    }
}

arq::DataPacketHeader arq::DataPacket::getHeader() const noexcept
{
    return header_;
}

void arq::DataPacket::setHeader(const DataPacketHeader& hdr)
{
    header_ = hdr;
    setDataLength(hdr.length_);
}


void arq::DataPacket::setDataLength(const size_t len)
{
    if (len > DATA_PKT_MAX_SIZE) {
        util::logWarning("DataPacket truncated to {} bytes", DATA_PKT_MAX_SIZE);
        header_.length_ = DATA_PKT_MAX_SIZE;
    }
    else {
        header_.length_ = len;
    }

    data_.resize(header_.size() + header_.length_);
    [[maybe_unused]] auto ret = serialiseHeader(); 
    assert(ret);
}

std::span<std::byte> arq::DataPacket::getSpan() noexcept
{
    return std::span<std::byte>(data_);
}

std::span<std::byte> arq::DataPacket::getHeaderSpan() noexcept
{
    return std::span<std::byte>(data_).subspan(0, header_.size());
}

std::span<std::byte> arq::DataPacket::getPayloadSpan() noexcept
{
    return std::span<std::byte>(data_).subspan(header_.size());
}

std::span<const std::byte> arq::DataPacket::getReadSpan() const noexcept
{
    return std::span<const std::byte>(data_);
}

std::span<const std::byte> arq::DataPacket::getHeaderReadSpan() const noexcept
{
    return std::span<const std::byte>(data_).subspan(0, header_.size());
}

std::span<const std::byte> arq::DataPacket::getPayloadReadSpan() const noexcept
{
    return std::span<const std::byte>(data_).subspan(header_.size());
}

bool arq::DataPacket::serialiseHeader() noexcept
{
    return header_.serialise(getHeaderSpan());
}

bool arq::DataPacket::deserialiseHeader() noexcept
{
    return header_.deserialise(getHeaderReadSpan());
}
