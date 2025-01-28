#include <catch2/catch_test_macros.hpp>

#include "arq/resequencing_buffers/stop_and_wait_rs.hpp"

// Returns a DataPacket with the given sequence number
auto get_data_packet(arq::SequenceNumber sn)
{
    arq::DataPacket pkt{};
    pkt.updateSequenceNumber(sn);
    return pkt;
}

TEST_CASE("Stop and wait RS buffer - add packets", "[arq/rs_buffers]")
{
    constexpr arq::SequenceNumber first_seq_num_to_add = 100;

    arq::rs::StopAndWait rs_buffer{first_seq_num_to_add}; // Ensure first SN is expected by RS

    REQUIRE_FALSE(rs_buffer.packetsPending());
    REQUIRE_FALSE(rs_buffer.getNextPacket().has_value());

    // Add a packet to the RS buffer
    auto ack = rs_buffer.addPacket(get_data_packet(first_seq_num_to_add));

    // Check an appropriate ACK is generated
    REQUIRE(ack.has_value());
    REQUIRE(ack.value() == first_seq_num_to_add);

    // Check buffer is full
    REQUIRE(rs_buffer.packetsPending());

    // Check another packet can't be added whilst buffer is full
    constexpr arq::SequenceNumber second_seq_num_to_add = 200;
    ack = rs_buffer.addPacket(get_data_packet(second_seq_num_to_add));
    REQUIRE_FALSE(ack.has_value());

    // Check buffer is still full
    REQUIRE(rs_buffer.packetsPending());
}

TEST_CASE("Stop and wait RS buffer - remove packets", "[arq/rs_buffers]")
{
    constexpr arq::SequenceNumber first_seq_num_to_add = 100;

    arq::rs::StopAndWait rs_buffer{first_seq_num_to_add}; // Ensure first SN is expected by RS

    REQUIRE_FALSE(rs_buffer.packetsPending());
    REQUIRE_FALSE(rs_buffer.getNextPacket().has_value());

    // Add a packet to the RS buffer
    auto ack = rs_buffer.addPacket(get_data_packet(first_seq_num_to_add));

    // Check an appropriate ACK is generated
    REQUIRE(ack.has_value());
    REQUIRE(ack.value() == first_seq_num_to_add);

    // Check buffer is full
    REQUIRE(rs_buffer.packetsPending());

    // Remove packet from RS
    auto pkt = rs_buffer.getNextPacket();
    REQUIRE(pkt.has_value());
    REQUIRE(pkt->getHeader().sequenceNumber_ == first_seq_num_to_add);

    // Check buffer is empty
    REQUIRE_FALSE(rs_buffer.packetsPending());
    pkt = rs_buffer.getNextPacket();
    REQUIRE_FALSE(pkt.has_value());

    // Try to add the next packet
    constexpr arq::SequenceNumber second_seq_num_to_add = first_seq_num_to_add + 1;
    ack = rs_buffer.addPacket(get_data_packet(second_seq_num_to_add));
    REQUIRE(ack.has_value());
    REQUIRE(ack.value() == second_seq_num_to_add);

    // Check buffer is full
    REQUIRE(rs_buffer.packetsPending());

    // Remove packet from RS
    pkt = rs_buffer.getNextPacket();
    REQUIRE(pkt.has_value());
    REQUIRE(pkt->getHeader().sequenceNumber_ == second_seq_num_to_add);

    // Check buffer is empty
    REQUIRE_FALSE(rs_buffer.packetsPending());
    pkt = rs_buffer.getNextPacket();
    REQUIRE_FALSE(pkt.has_value());

    // Try to add a packet which is not the next packet
    ack = rs_buffer.addPacket(get_data_packet(second_seq_num_to_add + 12));
    REQUIRE_FALSE(ack.has_value());
}

// To do: functional test with a sequence of packets

// To do: loss of ack test - check ACK is generated again
