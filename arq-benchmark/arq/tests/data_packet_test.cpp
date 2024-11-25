#include <catch2/catch_test_macros.hpp>

#include "arq/data_packet.hpp"
#include "util/logging.hpp"

static void data_packet_header_serialisation() {
    util::Logger::setLoggingLevel(util::LoggingLevel::LOGGING_LEVEL_DEBUG);

    // Initialise a header with random data
    arq::DataPacketHeader hdr_before {
        .id_ = 0x4F,
        .sequenceNumber_ = 0xE810,
        .length_ = 0x13C2
    };

    // Check that packed size is no bigger than unpacked size
    REQUIRE(hdr_before.size() <= sizeof(hdr_before));

    std::vector<std::byte> buffer(hdr_before.size() - 1);
    
    // Cannot serialise when buffer is too small
    REQUIRE(hdr_before.serialise(buffer) == false);

    // Serialise the data
    buffer.resize(hdr_before.size());
    REQUIRE(hdr_before.serialise(buffer) == true);

    // Deserialise the data
    arq::DataPacketHeader hdr_after{};
    REQUIRE(hdr_after.deserialise(buffer) == true);

    // Check data is unchanged
    util::logDebug("hdr_before: id: {} sn: {} len: {}",
                   hdr_before.id_,
                   hdr_before.sequenceNumber_,
                   hdr_before.length_);
    util::logDebug("hdr_after: id: {} sn: {} len: {}",
                   hdr_after.id_,
                   hdr_after.sequenceNumber_,
                   hdr_after.length_);
    
    REQUIRE(hdr_before == hdr_after);
}

static void data_packet_serialisation() {
    util::Logger::setLoggingLevel(util::LoggingLevel::LOGGING_LEVEL_DEBUG);

    // Initialise a header indicative of an oversized packet
    arq::DataPacketHeader hdr_before {
        .id_ = 0x2C,
        .sequenceNumber_ = 0x2BB0,
        .length_ = 0x11D4
    };

    REQUIRE(hdr_before.length_ > arq::DATA_PKT_MAX_PAYLOAD_SIZE);
    arq::DataPacket packet_before(hdr_before);

    // Check that length is capped when a packet is made
    REQUIRE(hdr_before != packet_before.getHeader());

    arq::DataPacketHeader hdr_before_truncated {
        .id_ = hdr_before.id_,
        .sequenceNumber_ = hdr_before.sequenceNumber_,
        .length_ = arq::DATA_PKT_MAX_PAYLOAD_SIZE
    };

    REQUIRE(hdr_before_truncated == packet_before.getHeader());

/*     util::logDebug("packet_before serialised:");
    for (auto readSpan = packet_before.getReadSpan(); auto b : readSpan) {
        std::print("{} ", std::to_integer<uint8_t>(b));
    }
    std::println(""); */

    // Check that header can be extracted from serial data
    arq::DataPacketHeader hdr_extracted;
    hdr_extracted.deserialise(packet_before.getHeaderReadSpan());

    std::vector<std::byte> hdr_before_serialdata(hdr_before.size());
    hdr_before.serialise(hdr_before_serialdata);
    util::logDebug("hdr_before serialised:");
    for (auto b : hdr_before_serialdata) {
        std::print("{} ", std::to_integer<uint8_t>(b));
    }
    std::println("");

    util::logDebug("packet_before header serialised:");
    for (auto readSpan = packet_before.getHeaderReadSpan(); auto b : readSpan) {
        std::print("{} ", std::to_integer<uint8_t>(b));
    }
    std::println("");

    std::vector<std::byte> hdr_before_truncated_serialdata(hdr_before.size());
    hdr_before_truncated.serialise(hdr_before_truncated_serialdata);

    util::logDebug("hdr_before_truncated serialised:");
    for (auto b : hdr_before_truncated_serialdata) {
        std::print("{} ", std::to_integer<uint8_t>(b));
    }
    std::println("");

    util::logDebug("hdr_before: id: {} sn: {} len: {}",
                   hdr_before.id_,
                   hdr_before.sequenceNumber_,
                   hdr_before.length_);
    util::logDebug("hdr_before_truncated: id: {} sn: {} len: {}",
                   hdr_before_truncated.id_,
                   hdr_before_truncated.sequenceNumber_,
                   hdr_before_truncated.length_);
    util::logDebug("hdr_extracted: id: {} sn: {} len: {}",
                   hdr_extracted.id_,
                   hdr_extracted.sequenceNumber_,
                   hdr_extracted.length_);

    // Create a packet with non-maximal length and random data
    const size_t test_pkt_len = 256;

    arq::DataPacketHeader test_hdr_before {
        .id_ = 0x2C,
        .sequenceNumber_ = 0x2BB0,
        .length_ = 0x11D4
    };

    test_hdr_before.length_ = test_pkt_len;
    std::vector<std::byte> test_pkt_data(test_pkt_len);
    for (size_t i = 0 ; i < test_pkt_len ; ++i) {
        test_pkt_data[i] = std::byte(i * i - 123 * i); // pseudo-random data
    }

    // Check header has successfully been inserted
    arq::DataPacket test_pkt(test_hdr_before);
    arq::DataPacketHeader test_hdr_extracted;
    test_hdr_extracted.deserialise(test_pkt.getHeaderReadSpan());

    util::logDebug("test_hdr_before: id: {} sn: {} len: {}",
                   test_hdr_before.id_,
                   test_hdr_before.sequenceNumber_,
                   test_hdr_before.length_);
    util::logDebug("test_hdr_extracted: id: {} sn: {} len: {}",
                   test_hdr_extracted.id_,
                   test_hdr_extracted.sequenceNumber_,
                   test_hdr_extracted.length_);

    REQUIRE(test_hdr_before == test_hdr_extracted);

    // Construct a new packet from serialised data
    arq::DataPacket test_pkt_after1(test_pkt.getReadSpan());
    REQUIRE(test_pkt_after1 == test_pkt);

    // ...and the same again with the vector rvalue constructor
    auto pkt_span = test_pkt.getReadSpan();
    std::vector<std::byte> temp(pkt_span.size());
    temp.assign(pkt_span.begin(), pkt_span.end());
    // std::memcpy(temp.data(), pkt_span.data(), pkt_span.size());
    
    arq::DataPacket test_pkt_after2(std::move(temp));
    REQUIRE(test_pkt_after2 == test_pkt);
}

TEST_CASE( "DataPacketHeader serialisation", "[arq]" ) {
    data_packet_header_serialisation();
}

TEST_CASE( "DataPacket serialisation", "[arq]" ) {
    data_packet_serialisation();
}