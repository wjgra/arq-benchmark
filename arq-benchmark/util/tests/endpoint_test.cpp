#include <catch2/catch_test_macros.hpp>
#include <future>

#include "util/endpoint.hpp"
#include "util/logging.hpp"

// To do: proper thread exception handling

static int test_server(const bool authenticateClient) {
    util::Endpoint endpoint{"127.0.0.1",
                            "65534",
                            util::SocketType::TCP};

    REQUIRE(endpoint.listen(1));

    if (authenticateClient) {
        REQUIRE(endpoint.accept("127.0.0.1"));
    }
    else {
        REQUIRE(endpoint.accept());
    }
    return EXIT_SUCCESS; // Dummy return value for future
}

static int test_client() {
    util::Endpoint endpoint{"127.0.0.1",
                            "65535",
                            util::SocketType::TCP};

    REQUIRE(endpoint.connectRetry("127.0.0.1",
                                  "65534",
                                  util::SocketType::TCP,
                                  10,
                                  std::chrono::milliseconds(100)));

    return EXIT_SUCCESS; // Dummy return value for future
}

static void endpoint_tcp_connection_test(const bool authenticateClient) {
    util::Logger::setLoggingLevel(util::LOGGING_LEVEL_ERROR);

    auto server = std::async(std::launch::async, test_server, authenticateClient);
    auto client = std::async(std::launch::async, test_client);

    try {
        server.get();
    }
    catch (const std::exception& e ) {
        util::logError("Server exception: {}", e.what());
        REQUIRE(1 == 0);
    }

    try {
        client.get();
    }
    catch (const std::exception& e ) {
        util::logError("Client exception: {}", e.what());
        REQUIRE(1 == 0);
    }
}

TEST_CASE( "Endpoint TCP connection (no authentication)", "[util]" ) {
    endpoint_tcp_connection_test(false);
}

TEST_CASE( "Endpoint TCP connection (with authentication)", "[util]" ) {
    endpoint_tcp_connection_test(true);
}
