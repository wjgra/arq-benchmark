#include <catch2/catch_test_macros.hpp>

#include <ranges>

#include "arq/retransmission_buffers/go_back_n_rt.hpp"

// WJG: common this up with snw test?

// A timeout that should not expire
constexpr size_t large_timeout = 1'000'000'000;

// Returns a Tx buffer object with the given sequence number
auto get_tx_buffer_object(arq::SequenceNumber sn)
{
    arq::DataPacket pkt{};
    pkt.updateSequenceNumber(sn);
    return arq::TransmitBufferObject{.packet_ = std::move(pkt), .info_ = {.sequenceNumber_ = sn}};
}

bool try_add_packet(arq::rt::GoBackN& rt_buf, arq::SequenceNumber sn)
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

TEST_CASE("Go-Back-N - add packets", "[arq/rt_buffers]")
{
    constexpr uint16_t window_size = 20;
    constexpr arq::SequenceNumber first_seq_num_to_add = 100;
    arq::rt::GoBackN rt_buffer{window_size, std::chrono::milliseconds(large_timeout), first_seq_num_to_add};

    REQUIRE_FALSE(rt_buffer.packetsPending());

    // Add packets until the buffer is full
    for (const auto sn : std::views::iota(first_seq_num_to_add) | std::views::take(window_size)) {
        REQUIRE(rt_buffer.readyForNewPacket());

        // Add a packet
        REQUIRE(try_add_packet(rt_buffer, sn));

        // Check packet added correctly
        REQUIRE(rt_buffer.packetsPending());

        auto pkt_span = rt_buffer.tryGetPacketSpan();
        REQUIRE(pkt_span.has_value());

        arq::DataPacket first_pkt(pkt_span.value());
        REQUIRE(first_pkt.getHeader().sequenceNumber_ == sn);
    }

    // Attempt to add another packet whilst buffer is full
    REQUIRE_FALSE(try_add_packet(rt_buffer, first_seq_num_to_add + window_size));
}

TEST_CASE("Go-Back-N - remove packets", "[arq/rt_buffers]")
{
    constexpr uint16_t window_size = 20;
    constexpr arq::SequenceNumber first_seq_num_to_add = 100;
    arq::rt::GoBackN rt_buffer{window_size, std::chrono::milliseconds(large_timeout), first_seq_num_to_add};

    REQUIRE_FALSE(rt_buffer.packetsPending());

    // Try ACKing packets when buffer is empty
    for (const auto sn : std::views::iota(first_seq_num_to_add) | std::views::take(window_size)) {
        rt_buffer.acknowledgePacket(sn);
        REQUIRE_FALSE(rt_buffer.packetsPending());
    }

    // Add packets until the buffer is full
    for (const auto sn : std::views::iota(first_seq_num_to_add) | std::views::take(window_size)) {
        REQUIRE(try_add_packet(rt_buffer, sn));
    }

    // Attempt to add another packet whilst buffer is full
    REQUIRE_FALSE(try_add_packet(rt_buffer, first_seq_num_to_add + window_size));

    // Acknowledge all packets in buffer and verify it's empty
    for (const auto sn : std::views::iota(first_seq_num_to_add) | std::views::take(window_size)) {
        REQUIRE(rt_buffer.packetsPending());
        rt_buffer.acknowledgePacket(sn);
    }

    REQUIRE_FALSE(rt_buffer.packetsPending());
}

// Include an add/remove test