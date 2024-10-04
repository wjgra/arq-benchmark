#include <catch2/catch_test_macros.hpp>

#include <array>
#include <mutex>
#include <future>
#include <random>

#include "util/endpoint.hpp"
#include "util/logging.hpp"

std::string_view serverHost = "127.0.0.1";
std::string_view serverService = "65534";
std::string_view clientHost = "127.0.0.1";
std::string_view clientService = "65535";

constexpr size_t sizeof_sendBuffer = 100;

auto makeSendBuffer() {
    std::array<uint8_t, sizeof_sendBuffer> buffer;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint8_t> dist(0, UINT8_MAX);
    for (auto& element : buffer) {
        element = dist(mt);
    }
    return buffer;
}

static const auto sendBuffer = makeSendBuffer();

static int test_server_tcp(const bool authenticateClient, const bool send) {
    util::Endpoint endpoint{serverHost,
                            serverService,
                            util::SocketType::TCP};

    REQUIRE(endpoint.listen(1));

    if (authenticateClient) {
        REQUIRE(endpoint.accept(clientHost));
    }
    else {
        REQUIRE(endpoint.accept());
    }

    if (send) {
        REQUIRE(endpoint.send(sendBuffer) == sendBuffer.size());
    }

    return EXIT_SUCCESS; // Dummy return value for future
}

static int test_client_tcp(const bool receive) {
    util::Endpoint endpoint{clientHost,
                            clientService,
                            util::SocketType::TCP};

    REQUIRE(endpoint.connectRetry(serverHost,
                                  serverService,
                                  util::SocketType::TCP,
                                  10,
                                  std::chrono::milliseconds(100)));

    if (receive) {
        std::array<uint8_t, sizeof_sendBuffer> recvBuffer{};
        REQUIRE(endpoint.recv(recvBuffer) == sendBuffer.size());

        // Check received data is correct
        REQUIRE(recvBuffer == sendBuffer);
    }

    return EXIT_SUCCESS; // Dummy return value for future
}

static void endpoint_tcp_connection_test(const bool authenticateClient,
                                         const bool sendReceive) {
    util::Logger::setLoggingLevel(util::LOGGING_LEVEL_ERROR);

    auto server = std::async(std::launch::async, test_server_tcp, authenticateClient, sendReceive);
    auto client = std::async(std::launch::async, test_client_tcp, sendReceive);

    try {
        client.get();
    }
    catch (const std::exception& e ) {
        util::logError("Client exception: {}", e.what());
        REQUIRE(1 == 0);
    }

    try {
        server.get();
    }
    catch (const std::exception& e ) {
        util::logError("Server exception: {}", e.what());
        REQUIRE(1 == 0);
    }
}

TEST_CASE( "Endpoint TCP connection (no authentication)", "[util]" ) {
    endpoint_tcp_connection_test(false, false);
}

TEST_CASE( "Endpoint TCP connection (with authentication)", "[util]" ) {
    endpoint_tcp_connection_test(true, false);
}

TEST_CASE( "Endpoint TCP send-receive", "[util]" ) {
    endpoint_tcp_connection_test(false, true);
}

constexpr size_t maxSendRecvAttempts = 100;

enum class UDPTestState {
    SERVER_TXING, // Server is tx'ing packet
    CLIENT_TXING, // Client has rx'd packet and is tx'ing ack
    END_TEST      // Server hsd rx'd ack
};

std::atomic<UDPTestState> udpTestState = UDPTestState::SERVER_TXING;

static int test_server_udp() {
    util::logDebug("Starting UDP test server");
    util::Endpoint endpoint{serverHost,
                            serverService,
                            util::SocketType::UDP};

    REQUIRE(udpTestState == UDPTestState::SERVER_TXING);

    // Send packet to client
    size_t attempt = 0;
    while (attempt < maxSendRecvAttempts && udpTestState == UDPTestState::SERVER_TXING) {
        auto ret = endpoint.sendTo(sendBuffer, clientHost, clientService);
        if (ret > 0) {
            REQUIRE(ret == sizeof_sendBuffer);
        }
        ++attempt;
    }
    REQUIRE(attempt <= maxSendRecvAttempts);
    REQUIRE(udpTestState == UDPTestState::CLIENT_TXING);
    
    // Listen for ack from client
    attempt = 0;
    while (attempt < maxSendRecvAttempts && udpTestState == UDPTestState::CLIENT_TXING) {
        std::array<uint8_t, sizeof_sendBuffer> recvBuffer{};
        auto ret = endpoint.recvFrom(recvBuffer);
        if (ret > 0) {
            // Ack received
            REQUIRE(recvBuffer == sendBuffer);
            udpTestState = UDPTestState::END_TEST;
        }
    }

    REQUIRE(attempt <= maxSendRecvAttempts);
    REQUIRE(udpTestState == UDPTestState::END_TEST);

    return EXIT_SUCCESS; // Dummy return value for future
}

static int test_client_udp() {
    util::logDebug("Starting client!");
    util::Endpoint endpoint{clientHost,
                            clientService,
                            util::SocketType::UDP};
    
    // Listen for packet from server
    std::array<uint8_t, sizeof_sendBuffer> recvBuffer{};
    size_t attempt = 0;
    while (attempt < maxSendRecvAttempts && udpTestState == UDPTestState::SERVER_TXING) {
        auto ret = endpoint.recvFrom(recvBuffer);
            if (ret > 0) {
                REQUIRE(recvBuffer == sendBuffer);
                udpTestState = UDPTestState::CLIENT_TXING;
            }
        ++attempt;
    }
    REQUIRE(attempt <= maxSendRecvAttempts);
    REQUIRE(udpTestState == UDPTestState::CLIENT_TXING);

    // Send ack to server
    attempt = 0;
    while (attempt < maxSendRecvAttempts && udpTestState == UDPTestState::CLIENT_TXING) {
        auto ret = endpoint.sendTo(recvBuffer, serverHost, serverService);
        if (ret > 0) {
            REQUIRE( ret == sizeof_sendBuffer);
        }
        ++attempt;
    }
    REQUIRE(attempt <= maxSendRecvAttempts);
    REQUIRE(udpTestState == UDPTestState::END_TEST);

    return EXIT_SUCCESS; // Dummy return value for future
}

static void endpoint_udp_connection_test() {
    util::Logger::setLoggingLevel(util::LOGGING_LEVEL_DEBUG);

    auto server = std::async(std::launch::async, test_server_udp);
    auto client = std::async(std::launch::async, test_client_udp);

    try {
        client.get();
    }
    catch (const std::exception& e ) {
        util::logError("Client exception: {}", e.what());
        REQUIRE(1 == 0);
    }

    try {
        server.get();
    }
    catch (const std::exception& e ) {
        util::logError("Server exception: {}", e.what());
        REQUIRE(1 == 0);
    }
}

TEST_CASE( "Endpoint UDP send-receive", "[util]" ) {
    endpoint_udp_connection_test();
    // To do: add test for overload of UDP sendTo
}
