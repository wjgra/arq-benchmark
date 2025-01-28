#include <catch2/catch_test_macros.hpp>

#include "arq/resequencing_buffers/go_back_n_rs.hpp"
#include "arq/resequencing_buffers/tests/rs_tests_common.hpp"

TEST_CASE("Go-Back-N RS buffer - add packets in order", "[arq/rs_buffers]")
{
    arq::rs::GoBackN rs_buffer{first_seq_num_to_add};
    add_packets_in_order_test(rs_buffer);
}

TEST_CASE("Go-Back-N RS buffer - add packets out of order", "[arq/rs_buffers]")
{
    arq::rs::GoBackN rs_buffer{first_seq_num_to_add};

    add_packets_out_of_order_test(rs_buffer);
}

TEST_CASE("Go-Back-N RS buffer - remove packets", "[arq/rs_buffers]")
{
    arq::rs::GoBackN rs_buffer{first_seq_num_to_add};
    remove_packets_test(rs_buffer);
}
