#include <catch2/catch_test_macros.hpp>

#include <ranges>

#include "arq/resequencing_buffers/dummy_sctp_rs.hpp"

// Returns a DataPacket with the given sequence number
auto get_data_packet(arq::SequenceNumber sn)
{
    arq::DataPacket pkt{};
    pkt.updateSequenceNumber(sn);
    return pkt;
}

constexpr arq::SequenceNumber first_seq_num_to_add = 100;
constexpr uint16_t num_packets_to_add = 1000;

TEST_CASE("Dummy SCTP RS buffer - add packets in order", "[arq/rs_buffers]")
{
    arq::rs::DummySCTP rs_buffer{first_seq_num_to_add}; // Ensure first SN is expected by RS

    REQUIRE_FALSE(rs_buffer.packetsPending());
    REQUIRE_FALSE(rs_buffer.getNextPacket().has_value());

    // Add multiple packets to the buffer in order, none should be rejected

    for (const auto sn :
         std::views::iota(first_seq_num_to_add, static_cast<uint16_t>(first_seq_num_to_add + num_packets_to_add))) {
        // Add a packet to the RS buffer
        auto ack = rs_buffer.addPacket(get_data_packet(sn));

        // Check an appropriate ACK is generated
        REQUIRE(ack.has_value());
        REQUIRE(ack.value() == sn);

        // Check buffer contains packets
        REQUIRE(rs_buffer.packetsPending());
    }
}

TEST_CASE("Dummy SCTP RS buffer - add packets out of order", "[arq/rs_buffers]")
{
    arq::rs::DummySCTP rs_buffer{first_seq_num_to_add}; // Ensure first SN is expected by RS

    REQUIRE_FALSE(rs_buffer.packetsPending());
    REQUIRE_FALSE(rs_buffer.getNextPacket().has_value());

    // Add multiple packets to the buffer in order, none should be rejected
    for (const auto sn :
         std::views::iota(first_seq_num_to_add, static_cast<uint16_t>(first_seq_num_to_add + num_packets_to_add))) {
        // Add the correct packet to the RS buffer
        auto ack = rs_buffer.addPacket(get_data_packet(sn));

        // Check an appropriate ACK is generated
        REQUIRE(ack.has_value());
        REQUIRE(ack.value() == sn);

        // Try to add the same packet again
        ack = rs_buffer.addPacket(get_data_packet(sn));
        REQUIRE_FALSE(ack.has_value());

        // Try to add a packet that isn't due yet
        ack = rs_buffer.addPacket(get_data_packet(sn + 2));
        REQUIRE_FALSE(ack.has_value());

        // Try to add a packet that has already been due
        ack = rs_buffer.addPacket(get_data_packet(sn - 1));
        REQUIRE_FALSE(ack.has_value());
    }
}

TEST_CASE("Dummy SCTP RS buffer - remove packets", "[arq/rs_buffers]")
{
    arq::rs::DummySCTP rs_buffer{first_seq_num_to_add};

    // Add packets in one thread, then ensure they are pushed to OB in order by pulling them in another thread
    auto get_packets_from_output_buffer_thread = std::thread([&rs_buffer]() {
        for (const auto sn :
             std::views::iota(first_seq_num_to_add, static_cast<uint16_t>(first_seq_num_to_add + num_packets_to_add))) {
            bool pkt_received = false;
            while (!pkt_received) {
                auto pkt = rs_buffer.getNextPacket();
                if (pkt.has_value()) {
                    REQUIRE(pkt->getHeader().sequenceNumber_ == sn);
                    pkt_received = true;
                }
            }
        }
    });

    auto add_packets_to_rs_buffer_thread = std::thread([&rs_buffer]() {
        for (const auto sn :
             std::views::iota(first_seq_num_to_add, static_cast<uint16_t>(first_seq_num_to_add + num_packets_to_add))) {
            // Add a packet to the RS buffer
            auto ack = rs_buffer.addPacket(get_data_packet(sn));

            // Check an appropriate ACK is generated
            REQUIRE(ack.has_value());
            REQUIRE(ack.value() == sn);
        }
    });

    get_packets_from_output_buffer_thread.join();
    add_packets_to_rs_buffer_thread.join();

    REQUIRE_FALSE(rs_buffer.packetsPending());
    REQUIRE_FALSE(rs_buffer.getNextPacket().has_value());
}

// To do: functional test with a sequence of packets

// To do: loss of ack test - check ACK is generated again
