#include <array>
#include <catch2/catch_test_macros.hpp>

#include "arq/common/control_packet.hpp"
#include "util/logging.hpp"

TEST_CASE("ControlPacket serialisation", "[arq]")
{
    util::Logger::setLoggingLevel(util::LoggingLevel::LOGGING_LEVEL_DEBUG);

    // Check serialisation for invalid buffer
    std::array<std::byte, 1> tooSmall;
    REQUIRE(tooSmall.size() < sizeof(arq::ControlPacket));

    const arq::SequenceNumber testSN = 0x1234;
    arq::ControlPacket ctrlPkt{.sequenceNumber_ = testSN};
    REQUIRE(ctrlPkt.serialise(tooSmall) == false);

    // Check deserialisation for invalid buffer
    REQUIRE(ctrlPkt.deserialise(tooSmall) == false);
    REQUIRE(ctrlPkt.sequenceNumber_ == testSN); // SN is unchanged by failed deserialisation

    // Test serialisation and deserialisation for all possible sequence numbers
    static_assert(sizeof(arq::SequenceNumber) == 2);
    for (uint16_t i = 0;; ++i) {
        std::array<std::byte, 2> buffer;

        arq::ControlPacket testCtrlPkt{.sequenceNumber_ = i};
        // Serialise
        REQUIRE(testCtrlPkt.serialise(buffer) == true);

        // Set SN to incorrect value before deserialisation
        testCtrlPkt.sequenceNumber_ += 10;

        // Deserialise
        REQUIRE(testCtrlPkt.deserialise(buffer) == true);

        // Check value is unchanged
        REQUIRE(testCtrlPkt.sequenceNumber_ == i);

        if (i == UINT16_MAX) {
            break;
        }
    }
}
