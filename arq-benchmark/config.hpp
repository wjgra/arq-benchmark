#ifndef _ARQ_BENCHMARK_CONFIG_HPP_
#define _ARQ_BENCHMARK_CONFIG_HPP_

#include <array>
#include <optional>
#include <stdexcept>
#include <string_view>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace arq {

struct config_AddressInfo {
    std::string hostName;
    std::string serviceName;
};

enum class ArqProtocol { DUMMY_SCTP, STOP_AND_WAIT, GO_BACK_N, SELECTIVE_REPEAT };

static constexpr auto arqProtocolToString(ArqProtocol protocol)
{
    switch (protocol) {
        case ArqProtocol::DUMMY_SCTP:
            return "dummy-sctp";
        case ArqProtocol::STOP_AND_WAIT:
            return "stop-and-wait";
        case ArqProtocol::GO_BACK_N:
            return "go-back-n";
        case ArqProtocol::SELECTIVE_REPEAT:
            return "selective-repeat";
        default:
            return "";
    };
}

struct config_common {
    config_AddressInfo serverNames;
    config_AddressInfo clientNames;
    ArqProtocol arqProtocol;
};

struct config_txPkts {
    uint16_t num;
    uint16_t msInterval;
};

struct config_Server {
    bool doNotVerifyClientAddr;
    config_txPkts txPkts;
    uint16_t arqTimeout;
};

struct config_Client {};

struct config_Launcher {
    config_common common;
    std::optional<config_Server> server;
    std::optional<config_Client> client;
};

in_addr stringToSockaddrIn(std::string_view str)
{
    in_addr out;
    if (inet_aton(str.data(), &out) == 0) {
        throw std::invalid_argument("invalid IPv4 address provided");
    }
    return out;
}

} // namespace arq

#endif