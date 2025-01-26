#include <catch2/catch_test_macros.hpp>

#include "arq/retransmission_buffers/stop_and_wait_rt.hpp"

// A timeout that should not expire
constexpr size_t large_timeout = 1'000'000'000;

// Returns a Tx buffer object with the given sequence number
arq::TransmitBufferObject get_tx_buffer_object(arq::SequenceNumber sn)
{
    arq::DataPacket pkt{};
    pkt.updateSequenceNumber(sn);
    return arq::TransmitBufferObject{.packet_ = std::move(pkt), .info_ = {.sequenceNumber_ = sn}};
}

bool try_add_packet(arq::rt::StopAndWait& rt_buf, arq::SequenceNumber sn)
{
    bool success = true;
    try {
        rt_buf.addPacket(get_tx_buffer_object(sn));
    }
    catch (...) {
        success = false;
    }
    return success;
}

TEST_CASE("Stop and wait RT buffer - add packets", "[arq/rt_buffers]")
{
    arq::rt::StopAndWait rt_buffer{std::chrono::milliseconds(large_timeout)};

    REQUIRE_FALSE(rt_buffer.packetsPending());
    REQUIRE(rt_buffer.readyForNewPacket());

    arq::SequenceNumber first_seq_num_to_add = 100;

    // Add a packet
    REQUIRE(try_add_packet(rt_buffer, first_seq_num_to_add));

    // Check packet added correctly
    REQUIRE(rt_buffer.packetsPending());
    REQUIRE_FALSE(rt_buffer.readyForNewPacket());

    auto pkt_span = rt_buffer.tryGetPacketSpan();
    REQUIRE(pkt_span.has_value());

    arq::DataPacket first_pkt(pkt_span.value());
    REQUIRE(first_pkt.getHeader().sequenceNumber_ == first_seq_num_to_add);

    // Attempt to add another packet whilst buffer is full
    REQUIRE(!try_add_packet(rt_buffer, first_seq_num_to_add));
}

TEST_CASE("Stop and wait RT buffer - acknowledge packets", "[arq/rt_buffers]")
{
    arq::rt::StopAndWait rt_buffer{std::chrono::milliseconds(large_timeout)};

    REQUIRE_FALSE(rt_buffer.packetsPending());
    REQUIRE(rt_buffer.readyForNewPacket());

    arq::SequenceNumber first_seq_num_to_add = 100;
    // Add a packet
    REQUIRE(try_add_packet(rt_buffer, first_seq_num_to_add));

    REQUIRE(rt_buffer.packetsPending());
    REQUIRE_FALSE(rt_buffer.readyForNewPacket());

    // Acknowledge the correct SN
    rt_buffer.acknowledgePacket(first_seq_num_to_add);

    // Check no packet is in the buffer
    REQUIRE_FALSE(rt_buffer.packetsPending());
    REQUIRE(rt_buffer.readyForNewPacket());

    auto pkt_span = rt_buffer.tryGetPacketSpan();
    REQUIRE_FALSE(pkt_span.has_value());

    arq::SequenceNumber second_seq_num_to_add = 200;

    // Add another packet
    REQUIRE(try_add_packet(rt_buffer, second_seq_num_to_add));

    REQUIRE(rt_buffer.packetsPending());
    REQUIRE_FALSE(rt_buffer.readyForNewPacket());

    // Ack wrong SN
    rt_buffer.acknowledgePacket(first_seq_num_to_add);

    // Check packet is still in the buffer
    REQUIRE(rt_buffer.packetsPending());
    REQUIRE_FALSE(rt_buffer.readyForNewPacket());

    pkt_span = rt_buffer.tryGetPacketSpan();
    REQUIRE(pkt_span.has_value());

    arq::DataPacket pkt(pkt_span.value());
    REQUIRE(pkt.getHeader().sequenceNumber_ == second_seq_num_to_add);

    // Ack correct SN
    rt_buffer.acknowledgePacket(second_seq_num_to_add);

    // Check no packet is in the buffer
    REQUIRE_FALSE(rt_buffer.packetsPending());
    REQUIRE(rt_buffer.readyForNewPacket());

    pkt_span = rt_buffer.tryGetPacketSpan();
    REQUIRE_FALSE(pkt_span.has_value());

    // Check buffer is still empty after unneeded ACK
    rt_buffer.acknowledgePacket(second_seq_num_to_add);

    REQUIRE_FALSE(rt_buffer.packetsPending());
    REQUIRE(rt_buffer.readyForNewPacket());

    pkt_span = rt_buffer.tryGetPacketSpan();
    REQUIRE_FALSE(pkt_span.has_value());  
}
