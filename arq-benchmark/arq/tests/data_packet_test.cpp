#include <catch2/catch_test_macros.hpp>

#include "arq/data_packet.hpp"

static void data_packet_header_serialisation() {
    util::Logger::setLoggingLevel(util::LoggingLevel::LOGGING_LEVEL_DEBUG);

    // Initialise a header with random data
    arq::DataPacketHeader hdr_before {
        .id_ = 0x4F,
        .sequenceNumber_ = 0xE810,
        .length_ = 0x12
    };

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

TEST_CASE( "DataPacketHeader serialisation", "[arq]" ) {
    data_packet_header_serialisation();
}