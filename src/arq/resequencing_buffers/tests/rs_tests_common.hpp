#ifndef _ARQ_RESEQUENCING_BUFFERS_TESTS_RS_TESTS_COMMON_HPP_
#define _ARQ_RESEQUENCING_BUFFERS_TESTS_RS_TESTS_COMMON_HPP_

#include <catch2/catch_test_macros.hpp>

#include <ranges>

#include "arq/resequencing_buffers/dummy_sctp_rs.hpp"
#include "arq/resequencing_buffers/go_back_n_rs.hpp"

/* Both the SCTP RS buffer and Go-Back-N RS buffer accept packets in order, and can hold
 * an unlimited number of packets at a time. Common tests for these buffers are defined
 * here. There is a slight difference with ACKs, but otherwise all is the same. */

// Returns a DataPacket with the given sequence number
auto get_data_packet(arq::SequenceNumber sn)
{
    arq::DataPacket pkt{};
    pkt.updateSequenceNumber(sn);
    return pkt;
}

constexpr arq::SequenceNumber first_seq_num_to_add = 100;
constexpr uint16_t num_packets_to_add = 1000;

template <typename RSBuffer>
void add_packets_in_order_test(RSBuffer& rs_buffer)
{
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

template <typename RSBuffer>
void add_packets_out_of_order_test(RSBuffer& rs_buffer)
{
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

        if (std::is_same<RSBuffer, arq::rs::GoBackN>()) {
            REQUIRE(ack.has_value());
            REQUIRE(ack.value() == sn);
        }
        else if (std::is_same<RSBuffer, arq::rs::DummySCTP>()) {
            // WJG: there is an issue here that warrants further investigation. Truly, the 
            // dummy buffer should not return any ACKs.
            REQUIRE_FALSE(ack.has_value());
        }
        else {
            // This function should not be used with any other buffers
            REQUIRE_FALSE(true);
        }

        // Try to add a packet that isn't due yet
        ack = rs_buffer.addPacket(get_data_packet(sn + 2));
        
        if (std::is_same<RSBuffer, arq::rs::GoBackN>()) {
            REQUIRE(ack.has_value());
            REQUIRE(ack.value() == sn);
        }
        else if (std::is_same<RSBuffer, arq::rs::DummySCTP>()) {
            // WJG: there is an issue here that warrants further investigation. Truly, the 
            // dummy buffer should not return any ACKs.
            REQUIRE_FALSE(ack.has_value());
        }
        else {
            // This function should not be used with any other buffers
            REQUIRE_FALSE(true);
        }

        // Try to add a packet that has already been due
        ack = rs_buffer.addPacket(get_data_packet(sn - 1));
        
        if (std::is_same<RSBuffer, arq::rs::GoBackN>()) {
            REQUIRE(ack.has_value());
            REQUIRE(ack.value() == sn);
        }
        else if (std::is_same<RSBuffer, arq::rs::DummySCTP>()) {
            // WJG: there is an issue here that warrants further investigation. Truly, the 
            // dummy buffer should not return any ACKs.
            REQUIRE_FALSE(ack.has_value());
        }
        else {
            // This function should not be used with any other buffers
            REQUIRE_FALSE(true);
        }
    }
}

template <typename RSBuffer>
void remove_packets_test(RSBuffer& rs_buffer)
{
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

#endif
