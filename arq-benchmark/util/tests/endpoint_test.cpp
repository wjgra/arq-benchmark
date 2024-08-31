#include <catch2/catch_test_macros.hpp>
#include <thread>

#include "util/endpoint.hpp"

// To do: use thread exception catcher (which needs its own tests...)

static void test_server(bool authenticateClient) {
    util::Endpoint endpoint{"127.0.0.1",
                            "65534",
                            util::SocketType::TCP};

    REQUIRE(endpoint.listen(50));

    if (authenticateClient) {
        // To implement when authentication implemented
    }
    else {
        REQUIRE(endpoint.accept());
    }
}

static void test_client() {
    util::Endpoint endpoint{"127.0.0.1",
                            "65535",
                            util::SocketType::TCP};

    REQUIRE(endpoint.connect("127.0.0.1",
                             "65534",
                             util::SocketType::TCP));
}

TEST_CASE( "Endpoint TCP connection", "[util]" ) {
    try {
        std::thread server{test_server, false};
        usleep(1000);
        std::thread client{test_client};
        server.join();
        client.join();
    }
    catch (const std::exception& e ) {
        REQUIRE(1 == 0);
    }
}
